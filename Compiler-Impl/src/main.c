#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <pthread.h>

int global = 0;
extern int getpid();
extern int printf(const char*, ...);

int test_branch(int a, int b)
{
	global ++;
	if(a == b)return 0;
	else if(a < b)return -1;
	else return 1;
}

__attribute__((noinline))
int tail_func(int a, int b, int c, int d)
{
	char arr[20];
	arr[0] = 0;
	return a + getpid() + b - getpid() + c + d - getpid();
}

int test_tail(int a, int b, int c, int d)
{
	char arr[20];
	arr[0] = a+b-c+c-b;
	a += getpid();
	return tail_func(arr[0], b, c, d);
}

__attribute__((noinline))
int sum_tail(int n, int p)
{
	if(n <= 0)return p;
	return sum_tail(n-1, p+n);
}


void test_params(int a, int b, int c, int d)
{
	char arr[20];
	arr[0] = 0;
	assert(a == 1);
	assert(b == 2);
	assert(c == 3);
	assert(d == 4);
}

void *small_return()
{
	char arr[20];
	arr[0] = 0;
	return small_return;
}

typedef struct{
	void *low;
	void *high;
}BIG_STRUCT;

__int128_t big_return()
{
	char arr[20];
	arr[0] = 0;
	__int128_t ret = 0xECE;
	ret = ret << 64 | 0xE2EE;
	return ret;
}

void test_return()
{
	assert(small_return() == small_return);
	__int128_t big = big_return();
	assert((big << 64 >> 64) == 0xE2EE);
	assert((big>>64) == 0xECE);
}

void b(jmp_buf env)
{
	longjmp(env, 0);
}

void a(jmp_buf env)
{
	b(env);
}

void test_setjmp()
{
	jmp_buf env;
	int ret = setjmp(env);
	if(ret)return;
	a(env);
	return;
}

void test_setjmp_return()
{
	jmp_buf env;
	setjmp(env);
	return;
}
void *test(void *arg)
{
	printf("====TEST TAIL CALL====\n");
	sum_tail(10, 20);
	test_tail(1, 2, 3, 4);
	printf("OK\n");

	printf("====TEST PARAMS====\n");
	test_params(1, 2, 3, 4);
	printf("OK\n");

	printf("====TEST RETURN====\n");
	test_return();
	printf("OK\n");

	printf("====TEST SETJMP===\n");
	test_setjmp();
	printf("OK!\n");


	printf("====TEST SETJMP NO LONGJMP===\n");
	test_setjmp_return();
	printf("OK!\n");
	return 0;
}
//for testing compilers
int main(int argc, const char *argv[])
{
	if(argc == 2)
	{
		pthread_t thread;
		pthread_create(&thread, NULL, test, (void*)1);
		pthread_join(thread, NULL);
		return 0;
	}
	test(NULL);
	return 0;
}
