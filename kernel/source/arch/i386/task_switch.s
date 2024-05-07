; task_switch.s
;   Purpose: Scheduler task switching

global schedTaskSwitch
schedTaskSwitch:
    mov r8, rdi
    add r8, 40

    mov rax, qword [rdi+172]

    mov rsp, qword [rdi+24]
    push qword [rdi]
;    mov ax, word [r8+120]
    mov r15, qword [r8]
    mov r14, qword [r8+8]
    mov r13, qword [r8+16]
    mov r12, qword [r8+24]
    mov r11, qword [r8+32]
    mov r10, qword [r8+40]
    mov r9, qword [r8+48]
    mov rsi, qword [r8+56]
    mov rdi, qword [r8+64]
    mov rdx, qword [r8+72]
    mov rcx, qword [r8+80]
    mov rbx, qword [r8+88]
    mov rax, qword [r8+96]
    mov rbp, qword [r8+112]
    mov r8, qword [r8+104]
    ret

global schedIntersegmentTaskSwitch
schedIntersegmentTaskSwitch:
    mov r8, rdi
    mov rax, cr3
    cmp rax, qword [r8+164]

    mov rax, qword [rdi+172]
    fxrstor [rax]
    
    je .changedCr3
    mov rax, qword [r8+164]
    mov cr3, rax
.changedCr3:
    movzx rbx, word [r8+160]
    mov ax, word [r8+162]
    movzx rcx, ax
    or rbx, 3
    or rcx, 3

    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax

    mov rax, qword [r8+24]
    push rcx
    push rax
    pushf
    push rbx
    push qword [r8]

    add r8, 40
    mov r15, qword [r8]
    mov r14, qword [r8+8]
    mov r13, qword [r8+16]
    mov r12, qword [r8+24]
    mov r11, qword [r8+32]
    mov r10, qword [r8+40]
    mov r9, qword [r8+48]
    mov rsi, qword [r8+56]
    mov rdi, qword [r8+64]
    mov rdx, qword [r8+72]
    mov rcx, qword [r8+80]
    mov rbx, qword [r8+88]
    mov rax, qword [r8+96]
    mov rbp, qword [r8+112]
    mov r8, qword [r8+104]
    iretq
