/* Copyright 2011-2012 Nicholas J. Kain, licensed under standard MIT license */
.global __setjmp
.global _setjmp
.global setjmp
.type __setjmp,@function
.type _setjmp,@function
.type setjmp,@function

.global __sigsetjmp
.global _sigsetjmp
.global sigsetjmp
.type __sigsetjmp,@function
.type _sigsetjmp,@function
.type sigsetjmp,@function
.intel_syntax noprefix
#include <ShadowStackConstants.h>
__setjmp:
_setjmp:
setjmp:
__sigsetjmp:
_sigsetjmp:
sigsetjmp:
	/*
	mov rdx, 0x7fc7fc0ee000 
	//top of shadow stack
	mov rax, [rdx]
	//get RA
	mov rcx, [rsp] 
	mov [rax], rcx
	mov [rax+8], rsp
	add rax, 16
	mov [rdx], rax
	*/

	/* rdi is jmp_buf, move registers onto it */
	mov [rdi], rbx
	mov [rdi+8], rbp
	mov [rdi+16], r12
	mov [rdi+24], r13
	mov [rdi+32], r14
	mov [rdi+40], r15
	/* this is our rsp WITHOUT current ret addr */
	lea rdx, [rsp+8]
	mov [rdi+48], rdx
	/* save return addr ptr for new rip */
	mov rdx, [rsp]
	mov [rdi+56], rdx
	/*Always return 0*/
	xor rax, rax
	ret
