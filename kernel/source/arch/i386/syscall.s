; syscall.s
; Purpose: System call register translation layer
extern syscGenericHandler

extern schedGetCurrentThread
getStack:
    call schedGetCurrentThread
    cmp rax, 0
    jne .different
    ret

.different:
    mov rax, qword [rax+220]
    ret

global syscTranslate
syscTranslate:
    push rdi
    push r13
    push r10
    push r9
    push r8
    push rdx
    push rbx
    push rax
    mov rdi, rsp

    call getStack
    xchg rsp, rax

    push rax
    ; save the rest of registers
    push rcx
    push rsi
    push r11
    push r12
    push r14
    push r15
    push rbp

    call syscGenericHandler

    pop rbp
    pop r15
    pop r14
    pop r12
    pop r11
    pop rsi
    pop rcx
    pop rsp

    add rsp, 8 ; RAX discarded
    pop rbx
    pop rdx
    pop r8
    pop r9
    pop r10
    pop r13
    pop rdi

    o64 sysret