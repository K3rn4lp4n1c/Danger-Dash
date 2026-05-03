%include "asm_io.inc"

segment .data
        game_args     dq "help", "version", "players", "test"
        help_message  db "Usage: danger-dash [options]", 10, 0
        GAME_TITLE   db "Danger Dash", 10, 0
        GAME_VERSION db "0.9.9-beta", 10, 0
        play_err_msg  db "Error: Invalid player arguments. Usage: danger-dash players <count> <name:char> ...", 10, 0
        default_name  db "John Doe", 0

segment .bss
        argc       resd 1
        argv       resd 1
        game       resd 1 ; pointer to the game struct
        count      resd 1
        names      resd 4 ; pointer to array of player names
        characters resd 4 ; pointer to array of player characters

segment .text
        global  asm_main
        global  check_for_collision
        global  move_player
        global  GAME_TITLE
        global  GAME_VERSION

asm_main:
        push    ebp
        mov     ebp, esp
        ; ********** CODE STARTS HERE **********
        ; get argc and argv from the stack and store them in argc and argv variables
        mov     eax, [ebp + 8]
        mov     [argc], eax
        mov     eax, [ebp + 12]
        mov     [argv], eax
        mov     eax, [argc]
        cmp     eax, 1
        je      .default

.resolve_args:
        mov     eax, [argv]
        add     eax, 4 ; skip program name
        mov     eax, [eax] ; get first argument

        mov     esi, eax
        mov     edi, game_args ; 'help'
        mov     ecx, 4 ; length of "help"
        call    .compare_string_helper
        je      .print_help

        mov     esi, eax
        mov     edi, game_args + 8 ; 'version'
        mov     ecx, 7 ; length of "version"
        call    .compare_string_helper
        je      .print_version

        mov     esi, eax
        mov     edi, game_args + 16 ; 'players'
        mov     ecx, 7 ; length of "players"
        call    .compare_string_helper
        je      .get_players

        mov     esi, eax
        mov     edi, game_args + 24 ; 'test'
        mov     ecx, 4 ; length of "test"
        call    .compare_string_helper
        je      .test

.print_help:
        mov     eax, help_message
        call    print_string
        jmp     asm_end

.default:
        mov     dword [count], 1
        mov     dword [names], default_name
        mov     dword [characters], 0
        jmp     game_main

.compare_string_helper:
        push    ebp
        mov     ebp, esp

        cld
        repe    cmpsb

        pop     ebp
        ret

.print_version:
        mov     eax, GAME_VERSION
        call    print_string
        jmp     asm_end

.test:
        call    hello_world
        jmp     asm_end

.get_players:
        ; ./game.out players 2 Alice:B Bob:E ...
        mov     eax, [argv]
        add     eax, 8 ; skip program name and first argument
        mov     eax, [eax] ; get player count argument
        mov     edx, eax
        call    atoi
        call    print_int
        call    print_nl
        cmp     eax, 0
        jle     .get_players_err ; player count must be > 0
        mov     [count], eax ; store player count

        mov     esi, [argv]
        add     esi, 12 ; skip program name and first two arguments
        cmp     dword [argc], 3
        jle     .get_players_err ; need at least 3 arguments for players mode
        xor     edi, edi
        jmp     .get_players_loop

.get_players_err:
        mov     eax, play_err_msg
        call    print_string
        jmp     asm_end

.get_players_loop:
        cmp     edi, ecx
        jge     game_main
        mov     eax, [esi]         ; eax = argv string pointer
        mov     edx, eax          ; edx = scan pointer

.find_colon_loop:
        mov     bl, [edx]
        cmp     bl, 0
        je      .no_colon
        cmp     bl, ':'
        je      .have_colon
        inc     edx
        jmp     .find_colon_loop

.have_colon:
        cmp     byte [edx+1], 0    ; ensure char exists after ':'
        je      .no_colon         ; treat as no-char -> default
        mov     byte [edx], 0     ; terminate name at ':'
        mov     bl, [edx+1]       ; read character letter

        cmp     bl, 'B'
        je      .set_B
        cmp     bl, 'b'
        je      .set_B
        cmp     bl, 'E'
        je      .set_E
        cmp     bl, 'e'
        je      .set_E
        cmp     bl, 'M'
        je      .set_M
        cmp     bl, 'm'
        je      .set_M
        cmp     bl, 'Y'
        je      .set_Y
        cmp     bl, 'y'
        je      .set_Y
        jmp     .set_default

.set_B:
        mov     dword [characters + edi*4], 0
        jmp     .store_name
.set_E:
        mov     dword [characters + edi*4], 1
        jmp     .store_name
.set_M:
        mov     dword [characters + edi*4], 2
        jmp     .store_name
.set_Y:
        mov     dword [characters + edi*4], 3
        jmp     .store_name
.set_default:
        mov     dword [characters + edi*4], 0
        jmp     .store_name

.no_colon:
        mov     dword [characters + edi*4], 0

.store_name:
        mov     ebx, names
        mov     [ebx + edi*4], eax    ; names[edi] = pointer to NUL-terminated name
        add     esi, 4
        inc     edi
        jmp     .get_players_loop

asm_end:
        ; *********** CODE ENDS HERE ***********
        mov     eax, 0
        mov     esp, ebp
        pop     ebp
        ret

atoi:
        xor     eax, eax
.loop:
        movzx   ecx, byte [edx]
        inc     edx
        cmp     ecx, '0'
        jb      .done
        cmp     ecx, '9'
        ja      .done
        sub     ecx, '0'
        imul    eax, 10
        add     eax, ecx
        jmp     .loop
.done:
        ret

game_main:
        push    dword characters ; pass the characters array pointer
        push    dword names      ; pass the names array pointer
        push    dword [count]      ; pass the player count
        call    init_game
        add     esp, 12 ; clean up the stack after the call
        mov     [game], eax ; store the pointer to the game struct in the game variable

.await_game_start_or_quit:
        call    curses_getch
        cmp     al, 'q' ; check if the user wants to quit
        je      .end
        cmp     al, ' ' ; check if the user wants to start the game
        je      .start
        jmp     .await_game_start_or_quit

.start:
        push    dword [game]
        call    run_game
        add     esp, 4
        push    dword [game]
        call    end_game
        add     esp, 4
        call    curses_getch ; wait for user input before exiting
        cmp     al, 'q' ; check if the user wants to quit
        je      .end
        jmp     .start

.end:
        ; Clean up resources and exit the game
        push    dword [game]
        call    deinit_game
        add     esp, 4
        
        jmp     asm_end

check_for_collision:
        push    ebp
        mov     ebp, esp
        push    ebx
        push    esi

        mov     edx, [ebp + 8]    ; y
        mov     ebx, [ebp + 12]   ; x

        mov     eax, [game]       ; load pointer-to-Game (global 'game' bss)
        test    eax, eax
        jz      .no_collision
        mov     eax, [eax + 8]    ; Game->env - will break if Game layout changes
        test    eax, eax
        jz      .no_collision
        mov     eax, [eax + 12]   ; Environment->map (char **)
        test    eax, eax
        jz      .no_collision

        mov     esi, [eax + edx*4] ; row pointer = map[y]
        test    esi, esi
        jz      .no_collision

        mov     al, [esi + ebx]   ; map[y][x]
        cmp     al, ' '
        jne     .collision        ; non-space => collision

.no_collision:
        xor     eax, eax
        jmp     .done

.collision:
        mov     eax, 1

.done:
        pop     esi
        pop     ebx
        mov     esp, ebp
        pop     ebp
        ret

move_player:
        push    ebp
        mov     ebp, esp

        ; void move_player(int *, int, int, int, int, int)
        sub     esp, 24 ; allocate space for 5 ints and 1 int pointer: 6 * 4 = 24 bytes
        mov     eax, [ebp + 8]  ; int *new_yx
        mov     [ebp - 4 ], eax
        mov     eax, [ebp + 12] ; int key
        mov     [ebp - 8 ], eax
        mov     eax, [ebp + 16] ; int player_y
        mov     [ebp - 12], eax
        mov     eax, [ebp + 20] ; int player_x
        mov     [ebp - 16], eax
        mov     eax, [ebp + 24] ; int lines
        mov     [ebp - 20], eax
        mov     eax, [ebp + 28] ; int cols
        mov     [ebp - 24], eax

        ; check if the key is an arrow key and update new_yx accordingly
        mov     eax, [ebp - 8]
        cmp     eax, 'w' ; up
        je      .move_up
        cmp     eax, 's' ; down
        je      .move_down
        cmp     eax, 'a' ; left
        je      .move_left
        cmp     eax, 'd' ; right
        je      .move_right

.cleanup:
        ; move the new_yx values back to the caller
        mov     eax, [ebp - 12] ; player_y
        mov     edx, [ebp - 16] ; player_x
        mov     ecx, [ebp - 4]  ; new_yx
        mov     [ecx], eax      ; new_yx[0] = player_y
        mov     [ecx + 4], edx  ; new_yx[1] = player_x

        mov     esp, ebp
        pop     ebp
        ret

.move_up:
        ; y = (y == max_y - 1) ? max_y / 2 : y;
        mov     eax, [ebp - 20] ; lines
        dec     eax ; max_y - 1
        cmp     dword [ebp - 12], eax ; player_y == lines - 1
        jne     .cleanup
        mov     eax, [ebp - 20] ; lines
        shr     eax, 1 ; max_y / 2
        mov     [ebp - 12], eax ; player_y = max_y / 2
        jmp     .cleanup

.move_down:
        ; y = (y < max_y - 1) ? max_y - 1 : y
        mov     eax, [ebp - 20] ; lines
        dec     eax ; max_y - 1
        cmp     dword [ebp - 12], eax ; player_y < lines - 1
        jge     .cleanup
        mov     [ebp - 12], eax
        jmp     .cleanup

.move_left:
        ; x = (x > 1) ? x - 1 : x;
        cmp     dword [ebp - 16], 1 ; player_x > 1
        jle     .cleanup
        dec     dword [ebp - 16] ; player_x - 1
        jmp     .cleanup

.move_right:
        ; x = (x < max_x - 1) ? x + 1 : x;
        mov     eax, [ebp - 24] ; cols
        dec     eax ; max_x - 1
        cmp     dword [ebp - 16], eax ; player_x < cols - 1
        jge     .cleanup
        inc     dword [ebp - 16] ; player_x + 1
        jmp     .cleanup