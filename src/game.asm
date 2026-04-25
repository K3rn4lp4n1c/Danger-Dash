%include "asm_io.inc"

segment .data
        help_msg db "Usage: danger-dash [options]", 10, 0

segment .bss
        argc resd 1
        argv resd 1
        game resd 1 ; pointer to the game struct

segment .text
        global  asm_main
        global check_for_collision

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

.print_help:
        call    help
        jmp     asm_end

help:
        ; Print help message to the console
        mov     eax, help_msg
        call    print_string
        ret

game_main:
        push    ebp
        mov     ebp, esp
        call    init_game
        mov     [game], eax

.await_game_start_or_quit:
        push    dword [game]
        call    update_game
        add     esp, 4
        call    curses_getch
        cmp     al, 'q'
        je      game_end
        cmp     al, ' '
        je      .start_game
        jmp     .await_game_start_or_quit

.start_game:
        push    dword [game]
        call    run_game
        add     esp, 4
        jmp     game_end

game_end:
        mov     eax, [game]
        push    eax
        call    end_game
        add     esp, 4
        mov     eax, [game]
        push    eax
        call    deinit_game
        add     esp, 4
        mov     esp, ebp
        pop     ebp
        ret

check_for_collision:
        ; This function will check for collision between the player and obstacles
        ; It will return 1 if there is a collision, 0 otherwise
        ; For now, we will just return 0 (no collision)
        xor     eax, eax
        ret