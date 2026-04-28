%include "asm_io.inc"

segment .data
        help_msg db "Usage: danger-dash [options]", 10, 0

segment .bss
        argc resd 1
        argv resd 1
        game resd 1 ; pointer to the game struct

segment .text
        global  asm_main
        global  check_for_collision
        global  move_player

asm_main:
        push    ebp
        mov     ebp, esp
        ; ********** CODE STARTS HERE **********
        ; get argc and argv from the stack and store them in argc and argv variables
        call    get_args
        call    game_main

asm_end:
        ; *********** CODE ENDS HERE ***********
        mov     eax, 0
        mov     esp, ebp
        pop     ebp
        ret

get_args:
        mov     eax, [ebp + 8]
        mov     [argc], eax
        mov     eax, [ebp + 12]
        mov     [argv], eax
        mov     eax, [argc]
        cmp     eax, 1
        jnle      .resolve_args
        ret

.resolve_args:
        mov     eax, [argv]
        mov     eax, [eax + 4] ; skip the first argument (program name)
        mov     al, [eax + 1] ; get the second character of the first argument
        cmp     al, 'h' ; check if it's 'h' for help
        je      .print_help
        ret

.print_help:
        call    help
        jmp     asm_end

help:
        ; Print help message to the console
        mov     eax, help_msg
        call    print_string
        ret

game_main:
        call    init_game
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