;------------------------------------------
; res = num1 - num2
; 	edi -- address of num1
; 	esi -- address of num2
; result:
;	edx -- address of res
sub:
	pusha

	mov  	eax, 0  	; al表示借位
	mov  	ebx, 0 		; bl--num1
	mov  	ecx, 0  	; cl--num2，ch记录循环次数
	mov   	byte[edx], 43	; 先默认结果为正
	push  	edx				; 先存一下edx

.loop:
    inc     edi
    inc     esi
    inc     edx
    inc     ch
    cmp     ch, 50		; 循环50次
    jg      .check
    mov     bl, byte [edi]
    mov     cl, byte [esi]
    sub  	bl, cl
    sub  	bl, al
    mov  	al, 0
    cmp  	bl, 0
    jnl  	.store
    mov  	al, 1  		; 借位
    add  	bl, 10

.store:
	mov  	byte[edx], bl
	jmp  	.loop

.check:
	pop  	edx			; 还原初始edx值。注意这行的位置
	cmp  	al, 1
	jne  	.finished
	; 结果为负数的情况
	; 最低位用10减，后面都用9减
	mov  	byte[edx], 45
	inc 	edx
	mov  	eax, 0
	mov  	ebx, 0
	mov  	ecx, 0
	mov 	bl, byte[edx]
	mov  	al, 10
	sub  	al, bl
	mov  	byte[edx], al

.reverse:
	mov  	al, 9
	inc  	edx
	inc  	cl
	cmp  	cl, 50
	jge  	.finished
	mov  	bl, byte[edx]
	sub  	al, bl
	mov  	byte[edx], al
	jmp  	.reverse

.finished:
    popa
    ret