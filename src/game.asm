%include "asm_io.inc"

segment .data
        help_msg db "Usage: danger-dash [options]", 10, 0

segment .bss
        argc resd 1
        argv resq 1

segment .text
        global  asm_main

asm_main:
        push    ebp
        mov     ebp, esp
        ; ********** CODE STARTS HERE **********
        ; get argc and argv from the stack
        call    get_args

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