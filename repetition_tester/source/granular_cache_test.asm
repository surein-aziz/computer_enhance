global Read_Granular

section .text

;
; NOTE(surein): These ASM routines are written for the Windows
; 64-bit ABI. They expect RCX to be the first parameter (kb of memory to test, divisible by 256b),
; RDX to be the second parameter (data pointer to 1gb+ buffer),
; R8 to be the third parameter (number of repetitions of sized read)
;

Read_Granular:
	align 64
.outer:
    xor rax, rax
    .inner:
        xor r9, r9
        add r9, rax
        add r9, rdx
        vmovdqu ymm0, [r9]
        vmovdqu ymm0, [r9 + 0x20]
        vmovdqu ymm0, [r9 + 0x40]
        vmovdqu ymm0, [r9 + 0x60]
        vmovdqu ymm0, [r9 + 0x80]
        vmovdqu ymm0, [r9 + 0xa0]
        vmovdqu ymm0, [r9 + 0xc0]
        vmovdqu ymm0, [r9 + 0xe0]
        add rax, 0x100
        cmp rax, rcx
        jb .inner
    dec r8
    jnz .outer
ret
