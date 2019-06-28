#define _GNU_SOURCE
#include "ShadowStackConstants.h"
#include "ShadowStackFeatures.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <pthread.h>
#include <errno.h>
#include <asm/prctl.h>
#include <sys/prctl.h>
#include <unistd.h>
#include "mpk.h"
int arch_prctl(int code, unsigned long addr);

typedef int (*pthread_create_f)(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);

typedef struct{
	void*(*start_routine)(void*);
	void *arg;
}FunctionCall;

static pthread_create_f real_pthread_create = NULL;
__attribute__((__noreturn__)) static void (*real_pthread_exit)(void *retval) = NULL;

void pthread_exit(void *retval)
{
	if(real_pthread_exit == NULL)real_pthread_exit = dlsym(RTLD_NEXT, "pthread_exit");
	pthread_attr_t attr;
	pthread_getattr_np(pthread_self(), &attr);
	void *stack_base;
	size_t stack_size;
	int status = 0;
	status = pthread_attr_getstack(&attr, &stack_base, &stack_size);
	if(status)
		errExit("get stack size");

	//retrieve shadow stack location
	asm volatile("movq %gs:8, %rax");
	asm volatile("movq %%rax, %0":"=rim"(stack_base));
	status = munmap(stack_base, stack_size);
	if(status == -1)
		errExit("munmap shadowstack");
	void *clave;
	asm volatile("movq %gs:32, %rax");
	asm volatile("movq %%rax, %0":"=rim"(clave));
	munmap(clave, getpagesize());
	real_pthread_exit(retval);
}

static void *getpage(void *ptr, size_t page_size)
{
	return mmap(ptr,
			page_size,
			PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}



static void init_shadow_stack(pthread_t thread)
{
	//get the start of stack memory and stack size
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_getattr_np(thread, &attr);
	void *stack_base;
	size_t stack_size;
	int status = 0;
	status = pthread_attr_getstack(&attr, &stack_base, &stack_size);
	if(status)
		errExit("get stack size");

	#ifdef SHADOW_STACK_MEM_SCHEME
		void **shadow_base = getpage(SHADOW_STACK_PTR, stack_size);
		if(shadow_base != SHADOW_STACK_PTR)
			errExit("Failed to mmap() at hardcoded location");
		*shadow_base = (shadow_base+1);
	#else

		//allocate the shadowstack at constant offset
	    void **shadow_base = getpage(
				(char*)stack_base+SHADOW_STACK_OFFSET, stack_size);
		if((char*)shadow_base != (char*)stack_base+SHADOW_STACK_OFFSET)
			errExit("Failed to mmap() at constant offset");
	#endif
    if(shadow_base == MAP_FAILED)
        errExit("failed to mmap() a shadow stack");

	#ifdef SHADOW_STACK_MPK
	    //bind mpk to the shadow stack
	    status = pkey_mprotect(shadow_base, stack_size,
	                           PROT_READ | PROT_WRITE, PKEY);
	    if (status == -1)
	        errExit("pkey_mprotect");
	#endif
}

#ifdef SHADOW_STACK_REG
static void init_shadow_stack_reg(pthread_t thread)
{
	//get the start of stack memory and stack size
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_getattr_np(thread, &attr);
	void *stack_base;
	size_t stack_size;
	int status = pthread_attr_getstack(&attr, &stack_base, &stack_size);
	if(status)
		errExit("get stack size");
	void **clave = mmap(NULL, getpagesize(),
						PROT_READ | PROT_WRITE,
						MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if(clave == MAP_FAILED)
		errExit("SHADOW_STACK_REG_CLAVE");

	void *shadowstack = getpage(NULL, stack_size);
	#ifdef SHADOW_STACK_MPK
		//bind mpk to the shadow stack
	    status = pkey_mprotect(shadowstack, stack_size,
	                           PROT_READ | PROT_WRITE, PKEY);
	    if (status == -1)
	        errExit("pkey_mprotect shadowstack");
	#endif
	#ifdef SHADOW_STACK_MPX
			mpxrt_prepare(stack_size);
	#endif
	clave[0] = (void*)((size_t)shadowstack - (size_t)stack_base);//(void*)((size_t)shadowstack + 16);
	//location of shadow stack
	clave[1] = shadowstack;
	//top of shadow stack, if using minimal style
	clave[2] = shadowstack;
	//function call count, if counting...
	clave[3] = 0;
	//location of this page
	clave[4] = (void*)clave;
	arch_prctl(ARCH_SET_GS, (unsigned long)(clave));
}
#endif

#ifdef SHADOW_STACK_SEG
static void init_shadow_stack_seg(pthread_t thread)
{
	//get the start of stack memory and stack size
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_getattr_np(thread, &attr);
	void *stack_base;
	size_t stack_size;
	int status = 0;
	status = pthread_attr_getstack(&attr, &stack_base, &stack_size);
	if(status)
		errExit("get stack size");
	void **page = getmem(NULL, stack_size, PROT_READ|PROT_WRITE, MAP_32BIT | MAP_PRIVATE | MAP_ANONYMOUS);
	if(page == MAP_FAIED)
		page = getpage(NULL, stack_size);
	if(page == MAP_FAILED)
		errExit("sub thread mmap()");
	page[0] = (void*)(page+2);
	page[1] = (void*)page;

	arch_prctl(ARCH_SET_GS, (unsigned long)(page));

	#ifdef SHADOW_STACK_MPK
		//bind mpk to the shadow stack
	    status = pkey_mprotect(page, stack_size,
	                           PROT_READ | PROT_WRITE, PKEY);
	    if (status == -1)
	        errExit("pkey_mprotect");
	#endif
}
#endif

static void* thread_init(void *ptr)
{
	FunctionCall *call = (FunctionCall*)ptr;	
#ifdef SHADOW_STACK_REG
	init_shadow_stack_reg(pthread_self());
	__asm__ volatile ("movq %gs:8, %r15");
#endif
#ifdef SHADOW_STACK_MEM_SCHEME
	init_shadow_stack(pthread_self());
#endif
#ifdef SHADOW_STACK_CON
	init_shadow_stack(pthread_self());
#endif
#ifdef SHADOW_STACK_SEG
	init_shadow_stack_seg(pthread_self());
#endif
	void *(*start_routine)(void*) = call->start_routine;
	void *arg = call->arg;
	free(call);
	void *ret = start_routine(arg);
	
	pthread_attr_t attr;
	pthread_getattr_np(pthread_self(), &attr);
	void *stack_base;
	size_t stack_size;
	int status = 0;
	status = pthread_attr_getstack(&attr, &stack_base, &stack_size);
	if(status)
		errExit("get stack size");

	//retrieve shadow stack location
	asm volatile("movq %gs:8, %rax");
	asm volatile("movq %%rax, %0":"=rim"(stack_base));
	status = munmap(stack_base, stack_size);
	void *clave;
	asm volatile("movq %gs:32, %rax");
	asm volatile("movq %%rax, %0":"=rim"(clave));
	munmap(clave, getpagesize());
	return ret;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg)
{
	if(real_pthread_create == NULL)real_pthread_create = dlsym(RTLD_NEXT, "pthread_create");

	FunctionCall *call = (FunctionCall*)malloc(sizeof(FunctionCall));
	call->start_routine = start_routine;
	call->arg = arg;

	int err = real_pthread_create(thread, attr, thread_init, (void*)call);
	if(err != 0)
	{
		free(call);
	}
	return err;
}

