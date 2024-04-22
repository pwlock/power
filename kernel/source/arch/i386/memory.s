global memset
memset:
    push rbp
    mov	rbp, rsp

    mov rax, rsi
    mov rcx, rdx
    rep stosb
    
    pop rbp
    mov rax, rsi
    ret

global _64memset
_64memset:
    push rbp
    mov	rbp, rsp

    mov rax, rsi
    mov rcx, rdx
    rep stosq
    
    pop rbp
    mov rax, rsi
    ret

global memcpy
memcpy:
    push rbp
    mov	rbp, rsp
    
    mov rcx, rdx
    rep movsb
    pop rbp
    mov rax, rsi
    ret
