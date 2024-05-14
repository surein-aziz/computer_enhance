global Random_Math
global Random_Math_Prefetch

section .text

;
; NOTE(surein): These ASM routines are written for the Windows
; 64-bit ABI. They expect RCX to be the first parameter (data pointer to 64kb+ buffer),
; RDX to be the second parameter (data pointer to 1gb+ buffer),
;

Random_Math:
	align 64
    xor r8, r8
    add r8, 0x2000
    xor r10, r10
    add r10, rdx
.outer:
    xor rax, rax
    .inner:
        xor r9, r9
        add r9, rax
        add r9, rcx
        vmovdqu ymm0, [r9]
        vmovdqu [r10], ymm0
        vmovdqu ymm0, [r9 + 0x20]
        vmovdqu [r10 + 0x20], ymm0
        vmovdqu ymm0, [r9 + 0x40]
        vmovdqu [r10 + 0x40], ymm0
        vmovdqu ymm0, [r9 + 0x60]
        vmovdqu [r10 + 0x60], ymm0
        vmovdqu ymm0, [r9 + 0x80]
        vmovdqu [r10 + 0x80], ymm0
        vmovdqu ymm0, [r9 + 0xa0]
        vmovdqu [r10 + 0xa0], ymm0
        vmovdqu ymm0, [r9 + 0xc0]
        vmovdqu [r10 + 0xc0], ymm0
        vmovdqu ymm0, [r9 + 0xe0]
        vmovdqu [r10 + 0xe0], ymm0
        add r10, 0x100
        add rax, 0x100
        cmp rax, 0x3FFF
        jb .inner
    dec r8
    jnz .outer
ret

Random_Math_Prefetch:
	align 64
    xor r8, r8
    add r8, 0x2000
    xor r10, r10
    add r10, rdx
.outer:
    xor rax, rax
    .inner:
        xor r9, r9
        add r9, rax
        add r9, rcx
        vmovdqu ymm0, [r9]
        vmovntdq [r10], ymm0
        vmovdqu ymm0, [r9 + 0x20]
        vmovntdq [r10 + 0x20], ymm0
        vmovdqu ymm0, [r9 + 0x40]
        vmovntdq [r10 + 0x40], ymm0
        vmovdqu ymm0, [r9 + 0x60]
        vmovntdq [r10 + 0x60], ymm0
        vmovdqu ymm0, [r9 + 0x80]
        vmovntdq [r10 + 0x80], ymm0
        vmovdqu ymm0, [r9 + 0xa0]
        vmovntdq [r10 + 0xa0], ymm0
        vmovdqu ymm0, [r9 + 0xc0]
        vmovntdq [r10 + 0xc0], ymm0
        vmovdqu ymm0, [r9 + 0xe0]
        vmovntdq [r10 + 0xe0], ymm0
        add r10, 0x100
        add rax, 0x100
        cmp rax, 0x3FFF
        jb .inner
    dec r8
    jnz .outer
ret
