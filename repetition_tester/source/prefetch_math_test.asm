global Random_Math
global Random_Math_Prefetch

section .text

;
; NOTE(surein): These ASM routines are written for the Windows
; 64-bit ABI. They expect RCX to be the first parameter (data pointer to 16mb+ buffer),
; RDX to be the second parameter (data pointer to 1gb+ buffer),
;

Random_Math:
	align 64
    xor rax, rax
.loop:
    xor r8, r8
    add r8, rax
    add r8, rcx
    mov r9, [r8]
    mov r10, [r8 + 8]
    mov r11, [r8 + 16]
    vmovdqu ymm0, [rdx + r9]
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmovdqu [rdx + r9], ymm0
    vmovdqu ymm0, [rdx + r10]
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmovdqu [rdx + r10], ymm0
    vmovdqu ymm0, [rdx + r11]
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmovdqu [rdx + r11], ymm0
    add rax, 24
    cmp rax, 0xFFFFFF
    jb .loop
ret

Random_Math_Prefetch:
	align 64
    xor rax, rax
.loop:
    xor r8, r8
    add r8, rax
    add r8, rcx
    mov r9, [r8]
    mov r10, [r8 + 8]
    mov r11, [r8 + 16]
    prefetcht0 [rdx + r10]
    prefetcht0 [rdx + r11]
    vmovdqu ymm0, [rdx + r9]
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmovdqu [rdx + r9], ymm0
    vmovdqu ymm0, [rdx + r10]
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmovdqu [rdx + r10], ymm0
    vmovdqu ymm0, [rdx + r11]
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmovdqu [rdx + r11], ymm0
    add rax, 24
    cmp rax, 0xFFFFFF
    jb .loop
ret
