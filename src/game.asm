%include "asm_io.inc"

segment .data


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