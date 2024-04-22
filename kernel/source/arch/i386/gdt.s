global gdtReloadSegments
gdtReloadSegments:
    push 3 * 8
    lea rax, [rel .rldcs]
    push rax
    o64 retf
.rldcs:
    mov ax, 4 * 8
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov ax, 0x38
    ltr ax
    ret
