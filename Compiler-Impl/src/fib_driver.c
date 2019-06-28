#include <stdio.h>
#include <x86intrin.h>

extern int fib(int);
extern int fib_mpk(int);


int main(int argc, const char *argv[])
{
	unsigned long long start = 0, end = 0;

	for(int i = 10; i < 50; i ++)
	{
		start = __rdtsc();
		fib(i);
		end = __rdtsc();
		unsigned long long base = (end - start);

		start = __rdtsc();
		fib_mpk(i);
		end = __rdtsc();
		unsigned long long mpk = (end - start);
		printf("i = %d, Overhead: %.2f%%\n", i, (((double)mpk/base)-1)*100);
	}
	return 0;
}
