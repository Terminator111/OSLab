;------------------------------------------
; int slen(String message)
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
 
 
;------------------------------------------
; void sprint(String message)
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
 
 
;------------------------------------------
; void sprintLF(String message)
; 输出字符串并换行
sprintLF:
    call    sprint
 
    push    eax
    mov     eax, 0Ah
    push    eax
    mov     eax, esp
    call    sprint
    pop     eax
    pop     eax
    ret


;------------------------------------------
; 格式化输出字符串并换行：
;   1、转换为大端存储
;   2、数字转为字符
;   3、是否输出符号
; eax--记录字符串的起始地址
; 思路：先push再pop
stdSprintLF:
    pusha

    push    eax         ; 存储初始地址
    mov     ecx, 0      ; 记录循环次数
    mov     ebx, 0
    inc     eax         ; 暂时不考虑符号位

.pushNext:
    inc     ecx
    cmp     ecx, 50     ; 循环50次
    jg      .popJudge
    mov     bl, byte[eax+ecx-1]
    push    ebx
    jmp     .pushNext

.popJudge:
    cmp     ecx, 1      
    je      .zero   ; 如果一直到最开始都是0，就直接输出0
    pop     ebx
    dec     ecx
    cmp     ebx, 0
    je      .popJudge   ; 忽略开头的0

.popNext:
    add     ebx, 48
    mov     byte[eax], bl
    dec     ecx
    cmp     ecx, 0
    je      .judgeSign
    pop     ebx
    inc     eax
    jmp     .popNext

.zero:
    mov     byte[eax], 48

.judgeSign:
    pop     eax             ; 恢复初始地址
    cmp     byte[eax], 43   ; 如果是加号就不用输出，让eax+1
    jne     .finished
    inc     eax

.finished:
    call    sprintLF
    popa
    ret


;------------------------------------------
; void exit()
; Exit program and restore resources
quit:
    mov     ebx, 0
    mov     eax, 1
    int     80h
    ret