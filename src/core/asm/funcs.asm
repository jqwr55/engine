global  maxofthree_asm
global  test_asm


maxofthree_asm:
    cmp     edi, esi
    cmovl   edi, esi
    cmp     edi, edx
    cmovl   edi, edx
    mov     eax, edi
    ret

test_asm:

    sub     rsp,20
    mov     r10,9
    mov     r11w,0b0000101001111000

loop:
    mov     WORD[rsp+r10*2],r11w
    sub     r10,1
    jne     loop


    mov     rax,1
    mov     rdi,1
    mov     rsi,rsp
    mov     rdx,20
    syscall 

    add     rsp,20

    mov rax,0
    ret

maxlines equ    8
dataSize equ    44