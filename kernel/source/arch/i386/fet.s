; fet.s
; Purpose: Enable CPU features (SSE, SYSCALL etc)

extern syscTranslate

enableAvx:
    push rax
    push rdx
    push rcx
    push r10

    xor ecx, ecx
    xor ebx, ebx
    mov eax, 0x1
    cpuid
    
    test ecx, 1 << 25 ; Test for AVX
    jnz .testXsave
    jmp .return
.testXsave:
    test ecx, 1 << 26 ; Test for XSAVE
    jz .return

.hasAvxXsave:
    xor r10w, r10w
    mov eax, 0x7
    xor ecx, ecx
    cpuid
    test ebx, 1 << 16 ; Test for AVX512
    jz .noAvxBit

    or r10w, 1      ; Has AVX512.

.noAvxBit:
    xor rcx, rcx
    xgetbv
    or eax, 7       ; Enable XCR0.X87, .SSE and .AVX
    bt r10w, 0

    jnc .noAvxToSet
    or eax, 0xe0    ; Enable XCR0.OPMASK, .ZMM_Hi256 and .Hi_ZMM16
.noAvxToSet:
    xsetbv

.return:            ; No support for AVX.
    pop r10
    pop rcx
    pop rdx
    pop rax
    ret

enableSse:
    push rax
    push r10
    xor r10w, r10w

    mov eax, 0x1
    cpuid
    bt ecx, 26
    jnc .noXsave
    or r10w, 1

.noXsave:
    test edx, 1 << 25
    jz .return ; No SSE

    mov rax, cr0
    and ax, 0xFFFB ; Disable CR0.EM
    or ax, 0x2     ; Set CR0.MP
    mov cr0, rax
    mov rax, cr4
    or eax, 3 << 9 ; Enable CR4.OSFXSR, CR4.OSXMMEXCPT and CR4.XSAVE

    bt r10w, 0
    jnc .noXsaveSet
    or eax, 1 << 18

.noXsaveSet
    mov cr4, rax

.return:
    pop r10
    pop rax
    jmp enableAvx

global cpuEnableFeatures
cpuEnableFeatures:
    push rax
    push rdx
    push rcx 
    push r10

    ; SYSCALL enabling
    mov rax, syscTranslate
    mov rdx, rax
    mov r10, 0xffffffff00000000
    and rdx, r10
    shr rdx, 32

    mov ecx, 0xC0000082         ; LSTAR
    wrmsr

    mov edx, ((0x18) | (0x28 << 16))
    xor rax, rax
    mov ecx, 0xC0000081         ; STAR
    wrmsr

    ; Enable EFER.SCA
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1
    wrmsr

    pop r10
    pop rcx
    pop rdx
    pop rax
    jmp enableSse
