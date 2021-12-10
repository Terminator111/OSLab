; Compile with: nasm -f elf main.asm
; Link with (64 bit systems require elf_i386 option): ld -m elf_i386 main.o -o main
; Run with: ./main

%include 'utils.asm'
; %include 'add.asm'    ; mul.asm中已经引用了，这里就不需要引用了
%include 'sub.asm'
%include 'mul.asm'

SECTION .data
inputPrompt db  'Please enter x and y: ', 0h

SECTION .bss
input:   resb    255
input1:  resb    255
input2:  resb    255
sum:     resb    255
product: resb    255

SECTION .text
global  _start
 
_start:
    ; 输出提示信息
    mov     eax, inputPrompt
    call    sprintLF
 
    ; 读取输入
    mov     edx, 255        ; 需要读取的字节数
    mov     ecx, input      ; 预留空间
    mov     ebx, 0          ; STDIN file
    mov     eax, 3          ; SYS_READ (kernel opcode 3)
    int     80h

    ; 拆分两个输入，并处理符号

    ; 先push一个0，作为开始标志，以便后续判断
    mov     eax, 0
    push    eax

    mov     eax, input
    ; edx存储input1的起始地址指针，ebx存储当前地址指针
    ; 先push再pop
    ; 小端存储
    mov     ebx, input1
    inc     ebx
    mov     edx, input1
    mov     byte[edx], 43   ; 在最前面添加加号
.pushNext:
    cmp     byte[eax], 10   ; 读到换行
    je      .popNext
    movzx   ecx, byte [eax]     
    push    ecx
    inc     eax
    jmp     .pushNext
.popNext:
    pop     ecx
    cmp     ecx, 0      ; 是否读到起始位置
    je      .judge      ; 处理输入完毕，判断符号
    cmp     ecx, 32     ; 是否读到空格
    je      .nextNum    ; 读到空格，就处理下一个数
    cmp     ecx, 45     ; 是否读到减号
    jne     .nextChar
    mov     byte[edx], 45   ; 把减号放在起始位置
    jmp     .popNext
.nextNum:
    mov     ebx, input2
    inc     ebx
    mov     edx, input2
    mov     byte [edx], 43  ; 在最前面添加加号
    pop     ecx
.nextChar:
    sub     cl, 48      ; 得到实际数值
    mov     byte [ebx], cl
    inc     ebx
    jmp     .popNext

.judge:
    mov     al, byte [input1]
    add     al, byte [input2]
    cmp     al, 88      ; 一加一减
    jne     toadd
    ; edi表示的数 - esi表示的数
    cmp     byte [input1], 45
    je      .exchange
    mov     edi, input1
    mov     esi, input2
    jmp     tosub
.exchange:
    mov     edi, input2
    mov     esi, input1
    jmp     tosub

toadd:
    mov     edi, input1
    mov     esi, input2
    mov     edx, sum
    call    add
    jmp     finish_add
tosub:
    mov     edx, sum
    call    sub

finish_add:
    ; 做乘法
    mov     edx, product
    call    mul
    
    ; 格式化并输出结果
    mov     eax, sum
    call    stdSprintLF
    mov     eax, product
    call    stdSprintLF

    call    quit
