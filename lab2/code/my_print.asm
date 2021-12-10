section .data
red:    db      1Bh, '[31;1m', 0
.len    equ     $ - red

white:  db      1Bh, '[37;1m', 0
.len    equ     $ - white

section .text
    global  my_print
    global  my_print_red

my_print:
    mov     eax, [esp+4]

    push    edx
    push    ecx
    push    ebx
    push    eax

    mov     eax, 4
    mov     ebx, 1
    mov     ecx, white
    mov     edx, white.len
    int     80h

    pop     eax
    pop     ebx
    pop     ecx
    pop     edx

    call    sprint
    ret

my_print_red:
    mov     eax, [esp+4]

    push    edx
    push    ecx
    push    ebx
    push    eax

    mov     eax, 4
    mov     ebx, 1
    mov     ecx, red
    mov     edx, red.len
    int     80h

    pop     eax
    pop     ebx
    pop     ecx
    pop     edx

    call    sprint
    ret

; 计算字符串长度
slen:
    push    ebx
    mov     ebx, eax

nextchar:
    cmp     byte [eax], 0
    jz      .finished
    inc     eax
    jmp     nextchar

.finished:
    sub     eax, ebx
    pop     ebx
    ret

; 输出字符串
sprint:
    push    edx
    push    ecx
    push    ebx
    push    eax
    call    slen

    mov     edx, eax
    pop     eax

    mov     ecx, eax
    mov     ebx, 1
    mov     eax, 4
    int     80h

    pop     ebx
    pop     ecx
    pop     edx
    ret
