global Read_1024mb
global Read_256mb
global Read_64mb
global Read_16mb
global Read_4mb
global Read_1024kb
global Read_256kb
global Read_64kb
global Read_16kb
global Read_4kb
global Read_1kb

section .text

;
; NOTE(surein): These ASM routines are written for the Windows
; 64-bit ABI. They expect RCX to be the first parameter (data pointer to 1gb+ buffer),
;

Read_1024mb:
    xor rax, rax
	align 64
.loop:
    xor rdx, rdx
    add rdx, rax
    and rdx, 0x3FFFFFFF ; NOTE(surein): 1024mb mask
    add rdx, rcx
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 0x100
    cmp rax, 0x3FFFFFFF
    jb .loop
    ret

Read_256mb:
    xor rax, rax
	align 64
.loop:
    xor rdx, rdx
    add rdx, rax
    and rdx, 0xFFFFFFF ; NOTE(surein): 256mb mask
    add rdx, rcx
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 0x100
    cmp rax, 0x3FFFFFFF
    jb .loop
    ret

Read_64mb:
    xor rax, rax
	align 64
.loop:
    xor rdx, rdx
    add rdx, rax
    and rdx, 0x3FFFFFF ; NOTE(surein): 64mb mask
    add rdx, rcx
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 0x100
    cmp rax, 0x3FFFFFFF
    jb .loop
    ret

Read_16mb:
    xor rax, rax
	align 64
.loop:
    xor rdx, rdx
    add rdx, rax
    and rdx, 0xFFFFFF ; NOTE(surein): 16mb mask
    add rdx, rcx
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 0x100
    cmp rax, 0x3FFFFFFF
    jb .loop
    ret

Read_4mb:
    xor rax, rax
	align 64
.loop:
    xor rdx, rdx
    add rdx, rax
    and rdx, 0x3FFFFF ; NOTE(surein): 4mb mask
    add rdx, rcx
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 0x100
    cmp rax, 0x3FFFFFFF
    jb .loop
    ret

Read_1024kb:
    xor rax, rax
	align 64
.loop:
    xor rdx, rdx
    add rdx, rax
    and rdx, 0xFFFFF ; NOTE(surein): 1024kb mask
    add rdx, rcx
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 0x100
    cmp rax, 0x3FFFFFFF
    jb .loop
    ret

Read_256kb:
    xor rax, rax
	align 64
.loop:
    xor rdx, rdx
    add rdx, rax
    and rdx, 0x3FFFF ; NOTE(surein): 256kb mask
    add rdx, rcx
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 0x100
    cmp rax, 0x3FFFFFFF
    jb .loop
    ret

Read_64kb:
    xor rax, rax
	align 64
.loop:
    xor rdx, rdx
    add rdx, rax
    and rdx, 0xFFFF ; NOTE(surein): 64kb mask
    add rdx, rcx
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 0x100
    cmp rax, 0x3FFFFFFF
    jb .loop
    ret

Read_16kb:
    xor rax, rax
	align 64
.loop:
    xor rdx, rdx
    add rdx, rax
    and rdx, 0x3FFF ; NOTE(surein): 16kb mask
    add rdx, rcx
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 0x100
    cmp rax, 0x3FFFFFFF
    jb .loop
    ret

Read_4kb:
    xor rax, rax
	align 64
.loop:
    xor rdx, rdx
    add rdx, rax
    and rdx, 0xFFF ; NOTE(surein): 4kb mask
    add rdx, rcx
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 0x100
    cmp rax, 0x3FFFFFFF
    jb .loop
    ret

Read_1kb:
    xor rax, rax
	align 64
.loop:
    xor rdx, rdx
    add rdx, rax
    and rdx, 0x3FF ; NOTE(surein): 1kb mask
    add rdx, rcx
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 0x100
    cmp rax, 0x3FFFFFFF
    jb .loop
    ret