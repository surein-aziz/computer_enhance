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
    xor rax, rax
    add rax, rcx
.loop:
    mov r8, [rax]
    vmovdqu ymm0, [rdx + r8]
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmovdqu [rdx + r8], ymm0
    mov r8, [rax + 8]
    vmovdqu ymm0, [rdx + r8]
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmovdqu [rdx + r8], ymm0
    add rax, 16
    cmp rax, 0xFFFF
    jb .loop
ret

Random_Math_Prefetch:
	align 64
    xor rax, rax
    add rax, rcx
.loop:
    mov r8, [rax]
    vmovdqu ymm0, [rdx + r8]
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmovdqu [rdx + r8], ymm0
    mov r8, [rax + 8]
    vmovdqu ymm0, [rdx + r8]
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmulpd ymm0, ymm0
    vmovdqu [rdx + r8], ymm0
    add rax, 16
    cmp rax, 0xFFFF
    jb .loop
ret
