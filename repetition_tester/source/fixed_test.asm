
global Read_Fixed

section .text

;
; NOTE(surein): These ASM routines are written for the Windows
; 64-bit ABI. They expect:
; RCX to be the first parameter (number of bits fixed)
; RDX to be the second parameter (data pointer to 1gb+ buffer)
; R8 to be the third parameter (number of reads)
;

Read_Fixed:
	align 64

    ; Set r10 to the step, keeping the required number of bits fixed
    xor r10, r10
    add r10, 0x1
    shl r10, 6
    shl r10, cl

    ; Set r9 to the base address
    xor r9, r9
    add r9, rdx

    ; Clear loop counter rax
    xor rax, rax
.loop:

    add r9, r10
    vmovdqu ymm0, [r9]
    add rax, 0x1
    cmp rax, r8
    jb .loop
    ret
