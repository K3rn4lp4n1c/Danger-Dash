%include "asm_io.inc"

segment .data
        help_msg       db "Usage: danger-dash [-p name character]...", 10
                       db "  -p <player_name> <character>  Add a player", 10
                       db "  -h, --help                    Show this help message", 10
                       db 10
                       db "Valid characters: Benjamin, Ethan, Muhammad, Youssef", 10, 0
        usage_msg      db "Error: Invalid arguments. Use -h for help.", 10, 0
        benjamin_str   db "Benjamin", 0
        ethan_str      db "Ethan", 0
        muhammad_str   db "Muhammad", 0
        youssef_str    db "Youssef", 0
        default_name   db "John Doe", 0

segment .bss
        game_ptr       resd 1
        player_count   resd 1
        names_array    resd 1
        chars_array    resd 1

segment .text
        global  asm_main
        global  check_for_collision
        global  move_player
        extern  strcmp, calloc, strdup, free
        extern  init_game, run_game, end_game, deinit_game, curses_getch

asm_main:
        push    ebp
        mov     ebp, esp
        
        ; Parse command line arguments
        mov     eax, [ebp + 8]      ; argc
        mov     ecx, [ebp + 12]     ; argv
        
        push    ecx
        push    eax
        call    parse_arguments
        add     esp, 8
        
        test    eax, eax
        jnz     .error_exit
        
        ; Initialize and run game
        call    run_game_loop
        
.error_exit:
        mov     eax, 0
        mov     esp, ebp
        pop     ebp
        ret

parse_arguments:
        push    ebp
        mov     ebp, esp
        
        mov     eax, [ebp + 8]      ; argc
        mov     ebx, [ebp + 12]     ; argv
        
        ; Initialize globals
        mov     dword [player_count], 0
        mov     dword [names_array], 0
        mov     dword [chars_array], 0
        
        ; Check if no arguments
        cmp     eax, 1
        je      .use_default
        
        ; Check for help
        mov     ecx, [ebx + 4]      ; argv[1]
        cmp     byte [ecx], '-'
        jne     .parse
        cmp     byte [ecx + 1], 'h'
        je      .show_help
        cmp     byte [ecx + 1], '-'
        jne     .parse
        cmp     byte [ecx + 2], 'h'
        je      .show_help
        
.parse:
        mov     ecx, 1              ; index = 1
        mov     edx, 0              ; player_idx = 0
        
.parse_loop:
        mov     eax, [ebp + 8]      ; argc
        cmp     ecx, eax
        jge     .done_parsing
        
        ; Get current argument
        mov     esi, [ebx + ecx*4]
        cmp     byte [esi], '-'
        jne     .invalid
        
        cmp     byte [esi + 1], 'p'
        jne     .invalid
        
        ; Found -p, get name
        inc     ecx
        cmp     ecx, [ebp + 8]
        jge     .invalid
        
        mov     esi, [ebx + ecx*4]  ; name string
        
        ; Duplicate name
        push    edx
        push    ecx
        push    esi
        call    strdup
        add     esp, 4
        pop     ecx
        pop     edx
        
        ; Store name
        mov     edi, [names_array]
        test    edi, edi
        jnz     .store_name
        
        ; Allocate names array (4 pointers)
        push    eax
        push    edx
        push    ecx
        push    4
        push    4
        call    calloc
        add     esp, 8
        mov     [names_array], eax
        pop     ecx
        pop     edx
        pop     eax
        
.store_name:
        mov     edi, [names_array]
        mov     [edi + edx*4], eax
        
        ; Get character
        inc     ecx
        cmp     ecx, [ebp + 8]
        jge     .invalid
        
        mov     esi, [ebx + ecx*4]  ; character string
        
        ; Convert to enum
        push    edx
        push    ecx
        push    esi
        call    string_to_character
        add     esp, 4
        pop     ecx
        pop     edx
        
        ; Store character
        mov     edi, [chars_array]
        test    edi, edi
        jnz     .store_char
        
        ; Allocate chars array (4 ints)
        push    eax
        push    edx
        push    ecx
        push    4
        push    4
        call    calloc
        add     esp, 8
        mov     [chars_array], eax
        pop     ecx
        pop     edx
        pop     eax
        
.store_char:
        mov     edi, [chars_array]
        mov     [edi + edx*4], eax
        
        ; Next player
        inc     edx
        inc     ecx
        jmp     .parse_loop
        
.invalid:
        mov     eax, usage_msg
        call    print_string
        mov     eax, 1
        jmp     .return
        
.show_help:
        mov     eax, help_msg
        call    print_string
        mov     eax, 1
        jmp     .return
        
.use_default:
        mov     dword [player_count], 1
        mov     dword [names_array], default_name
        mov     dword [chars_array], 0
        mov     eax, 0
        jmp     .return
        
.done_parsing:
        mov     [player_count], edx
        test    edx, edx
        jg      .success
        ; No players specified, use default
        mov     dword [player_count], 1
        mov     dword [names_array], default_name
        mov     dword [chars_array], 0
        mov     eax, 0
        jmp     .return
        
.success:
        mov     eax, 0
        
.return:
        mov     esp, ebp
        pop     ebp
        ret

string_to_character:
        push    ebp
        mov     ebp, esp
        mov     eax, [ebp + 8]
        
        push    eax
        push    benjamin_str
        call    strcmp
        add     esp, 8
        test    eax, eax
        je      .benjamin
        
        push    dword [ebp + 8]
        push    ethan_str
        call    strcmp
        add     esp, 8
        test    eax, eax
        je      .ethan
        
        push    dword [ebp + 8]
        push    muhammad_str
        call    strcmp
        add     esp, 8
        test    eax, eax
        je      .muhammad
        
        push    dword [ebp + 8]
        push    youssef_str
        call    strcmp
        add     esp, 8
        test    eax, eax
        je      .youssef
        
.benjamin:
        mov     eax, 0
        jmp     .char_done
.ethan:
        mov     eax, 1
        jmp     .char_done
.muhammad:
        mov     eax, 2
        jmp     .char_done
.youssef:
        mov     eax, 3
        jmp     .char_done
        
.char_done:
        mov     esp, ebp
        pop     ebp
        ret

run_game_loop:
        push    ebp
        mov     ebp, esp
        
        ; Call init_game(count, names, characters)
        push    dword [chars_array]
        push    dword [names_array]
        push    dword [player_count]
        call    init_game
        add     esp, 12
        mov     [game_ptr], eax
        
        cmp     eax, 0
        je      .error
        
        ; Wait for space or q
.wait_input:
        call    curses_getch
        cmp     al, 'q'
        je      .cleanup
        cmp     al, ' '
        je      .start_game
        jmp     .wait_input
        
.start_game:
        push    dword [game_ptr]
        call    run_game
        add     esp, 4
        
        push    dword [game_ptr]
        call    end_game
        add     esp, 4
        
        call    curses_getch
        cmp     al, 'q'
        je      .cleanup
        jmp     .start_game
        
.cleanup:
        push    dword [game_ptr]
        call    deinit_game
        add     esp, 4
        
.error:
        mov     esp, ebp
        pop     ebp
        ret

check_for_collision:
        push    ebp
        mov     ebp, esp
        push    ebx
        push    esi
        
        mov     edx, [ebp + 8]      ; y
        mov     ebx, [ebp + 12]     ; x
        
        mov     eax, [game_ptr]
        test    eax, eax
        jz      .no_collision
        
        mov     eax, [eax + 8]      ; Game->env
        test    eax, eax
        jz      .no_collision
        
        mov     eax, [eax + 12]     ; Environment->map
        test    eax, eax
        jz      .no_collision
        
        mov     esi, [eax + edx*4]  ; map[y]
        test    esi, esi
        jz      .no_collision
        
        mov     al, [esi + ebx]     ; map[y][x]
        cmp     al, ' '
        jne     .collision
        
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
        
        sub     esp, 24
        mov     eax, [ebp + 8]      ; int *new_yx
        mov     [ebp - 4], eax
        mov     eax, [ebp + 12]     ; int key
        mov     [ebp - 8], eax
        mov     eax, [ebp + 16]     ; int player_y
        mov     [ebp - 12], eax
        mov     eax, [ebp + 20]     ; int player_x
        mov     [ebp - 16], eax
        mov     eax, [ebp + 24]     ; int lines
        mov     [ebp - 20], eax
        mov     eax, [ebp + 28]     ; int cols
        mov     [ebp - 24], eax
        
        mov     eax, [ebp - 8]
        cmp     eax, 'w'
        je      .move_up
        cmp     eax, 's'
        je      .move_down
        cmp     eax, 'a'
        je      .move_left
        cmp     eax, 'd'
        je      .move_right
        
.cleanup:
        mov     eax, [ebp - 12]
        mov     edx, [ebp - 16]
        mov     ecx, [ebp - 4]
        mov     [ecx], eax
        mov     [ecx + 4], edx
        
        mov     esp, ebp
        pop     ebp
        ret
        
.move_up:
        mov     eax, [ebp - 20]
        dec     eax
        cmp     dword [ebp - 12], eax
        jne     .cleanup
        mov     eax, [ebp - 20]
        shr     eax, 1
        mov     [ebp - 12], eax
        jmp     .cleanup
        
.move_down:
        mov     eax, [ebp - 20]
        dec     eax
        cmp     dword [ebp - 12], eax
        jge     .cleanup
        mov     [ebp - 12], eax
        jmp     .cleanup
        
.move_left:
        cmp     dword [ebp - 16], 1
        jle     .cleanup
        dec     dword [ebp - 16]
        jmp     .cleanup
        
.move_right:
        mov     eax, [ebp - 24]
        dec     eax
        cmp     dword [ebp - 16], eax
        jge     .cleanup
        inc     dword [ebp - 16]
        jmp     .cleanup