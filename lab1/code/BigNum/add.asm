;------------------------------------------
; add two long number -- sum = num1 + num2
; 	edi -- address of num1
; 	esi -- address of num2
; result:
;	edx -- address of sum
add:
	pusha

	mov 	eax, 0 			; al保存进位
	mov  	ebx, 0 			; bl--num1
	mov  	ecx, 0  		; cl--num2，ch记录循环次数
	mov   	byte[edx], 43
	cmp 	byte[edi], 43	; 确定结果的符号
	je  	.loop
	mov 	byte[edx], 45

.loop:
    inc     edi
    inc     esi
    inc     edx
    inc     ch
    cmp     ch, 50		; 循环50次
    jg      .finished
    mov     bl, byte [edi]
    mov     cl, byte [esi]
    add     bl, al
    add     bl, cl
    mov     eax, ebx
    push    ebx
    mov     ebx, 10
    push    edx
    mov     edx, 0
    div     ebx			; eax除以ebx，商在eax，余数在edx
    mov     eax, edx
    pop     edx
    mov     byte [edx], al	; 将余数写入结果
    mov     al, 0
    pop     ebx

    cmp     bl, 9
    jng     .loop
    mov     al, 1 		; 有进位
    jmp     .loop

.finished:
    popa
    ret