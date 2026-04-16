%include "asm_io.inc"

segment .data


segment .bss


segment .text
        global  asm_main

asm_main:
        push    ebp
        mov     ebp, esp
        ; ********** CODE STARTS HERE **********
        call    hello_world       ; call the hello_world function from asm_io.asm
        ; *********** CODE ENDS HERE ***********
        mov     eax, 0
        mov     esp, ebp
        pop     ebp
        ret