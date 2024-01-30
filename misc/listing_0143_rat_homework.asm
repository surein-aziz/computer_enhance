;  ========================================================================
;
;  (C) Copyright 2023 by Molly Rocket, Inc., All Rights Reserved.
;
;  This software is provided 'as-is', without any express or implied
;  warranty. In no event will the authors be held liable for any damages
;  arising from the use of this software.
;
;  Please see https://computerenhance.com for more information
;
;  ========================================================================

;  ========================================================================
;  LISTING 143
;  ========================================================================

;
;  NOTE(casey): Regular Homework
;                       dest    source0 source1
    mov rax, 1          r0               
    mov rbx, 2          r1
    mov rcx, 3          r2
    mov rdx, 4          r3
    add rax, rbx        r4      r1      r0
    add rcx, rdx        r5      r3      r2
    add rax, rcx        r6      r5      r4
    mov rcx, rbx        r7      r1
    inc rax             r8      r6
    dec rcx             r9      r7
    sub rax, rbx        r10     r1      r8
    sub rcx, rdx        r11     r3      r9
    sub rax, rcx        r12     r11     r10

;
;  NOTE(casey): CHALLENGE MODE WITH ULTIMATE DIFFICULTY SETTINGS
;               DO NOT ATTEMPT THIS! IT IS MUCH TOO HARD FOR
;               A HOMEWORK ASSIGNMENT!1!11!!
;
top:
    pop rcx
    sub rsp, rdx
    mov rbx, rax
    shl rbx, 0
    not rbx
    loopne top
