/* Copyright 2011-2012 Nicholas J. Kain, licensed under standard MIT license */
#include <ShadowStackConstants.h>

.global __longjmp
.global _longjmp
.global longjmp
.global __libc_siglongjmp
.type __longjmp,@function
.type _longjmp,@function
.type longjmp,@function
.type __libc_siglongjmp,@function
.global __siglongjmp
.global _siglongjmp
.global siglongjmp
.type __siglongjmp,@function
.type _siglongjmp,@function
.type siglongjmp,@function
.intel_syntax noprefix

__longjmp:
_longjmp:
longjmp:
__libc_siglongjmp:
__siglongjmp:
_siglongjmp:
siglongjmp:
#xor eax, eax
#xor ecx, ecx
#xor edx, edx
#wrpkru

	mov r15, gs:[8] 
#mov rax, [rdx]
	lea r15, [r15+0x10]

	//get recoreded RSP with RA
	mov rcx, [rdi+48]
2:
	cmp qword ptr[r15], 0
	je 3f
	cmp rcx, [r15+8]
	jge 4f
	add r15, 16
	jmp 2b
3:
	int 3
4:
#mov gs:[0], r15
	mov rax, rsi       /* val will be longjmp return */
	test rax,rax
	jnz 1f
	inc rax                /* if val==0, val=1 per longjmp semantics */
1:
	/* rdi is the jmp_buf, restore regs from it */
	mov rbx, [rdi]
	mov rbp, [rdi+8]
	mov r12, [rdi+16]
	mov r13, [rdi+24]
	mov r14, [rdi+32]
	mov r15, [rdi+40]
	/* this ends up being the stack pointer */
	mov rdx, [rdi+48]
	mov rsp, rdx
	/* this is the instruction pointer */
	mov rdx, [rdi+56]
	/* goto saved address without altering rsp */
	jmp rdx
