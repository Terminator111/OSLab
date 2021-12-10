;------------------------------------------
; product = num1 * num2
; 	edi -- address of num1
; 	esi -- address of num2
; result:
;	edx -- address of product

%include 'add.asm'

SECTION .bss
tmp:   resb    255

SECTION .text
mul:
	pusha

	mov  	eax, 0
	mov  	ebx, 0
	mov  	ecx, 0
	mov  	byte[tmp], 43
	mov   	byte[edx], 43	; 先将结果设为正

.loop1:
	cmp  	ch, 50  	; 外层循环  50次
	jnl  	.sign
	cmp   	cl, 50  	; 内层循环  50次
	jl  	.loop2
	inc   	ch
	mov  	cl, 0

.loop2:
	movzx  	eax, cl
	add  	eax, edi
	movzx  	eax, byte[eax+1]	; num1当前数
	push  	ebx
	movzx   ebx, ch
	add  	ebx, esi
	movzx  	ebx, byte[ebx+1] 	; num2当前数
	push  	edx
	mul  	ebx		; 这一步会改变edx的值
	pop 	edx
	pop  	ebx

	push  	edx
	push    ecx
	mov    	edx, 0
	mov    	ecx, 10
	div  	ecx			; eax--商  edx--余数
	pop   	ecx
	push 	ecx
	add  	cl, ch
	movzx  	ecx, cl
	mov  	byte[tmp+ecx+1], dl
	mov   	byte[tmp+ecx+1+1], al
	pop 	ecx
	pop  	edx

	push 	edi
	push  	esi
	mov  	edi, edx
	mov  	esi, tmp
	call  	add
	
	; 将tmp清空
	push  	eax
	mov  	eax, 0
.loop:
	cmp     eax, 50         ; 循环50次
	jnl     .pop
	mov     byte[tmp+eax+1], 0
	inc  	eax
	jmp  	.loop
.pop:
	pop  	eax

	pop 	esi
	pop  	edi

	inc  	cl
	jmp  	.loop1

.sign:
	; 确定结果的符号
	mov  	al, byte[edi]
	add  	al, byte[esi]
	cmp  	al, 88 		; 异号
	jne  	.finished
	mov  	byte[edx], 45

.finished:
    popa
    ret