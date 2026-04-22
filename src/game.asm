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
        ; This is the main function for the game logic
        ; It will initialize the game, run the game loop, and clean up resources
        call    init_game
        mov     [game], eax ; store the game pointer returned by init_game
        push    dword [game] ; push the game pointer as an argument to run_game
        jmp    .await_game_start_or_quit

.await_game_start_or_quit:
        call    update_game
        call    curses_getch
        cmp     al, 'q' ; check if the user wants to quit
        je      game_end
        cmp     al, ' ' ; check if the user wants to start the game
        je      run_game
        jmp     .await_game_start_or_quit

game_end:
        ; Clean up resources and exit the game
        mov     eax, [game]
        push    eax ; push the game pointer as an argument to end_game
        call    end_game
        mov     eax, [game]
        push    eax ; push the game pointer as an argument to deinit_game
        call    deinit_game

check_for_collision:
        ; This function will check for collision between the player and obstacles
        ; It will return 1 if there is a collision, 0 otherwise
        ; For now, we will just return 0 (no collision)
        xor     eax, eax
        ret