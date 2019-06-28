#define _GNU_SOURCE
#include <stdio.h>

#define GS_RELATIVE __attribute__((address_space(256)))
static char read_gs(char GS_RELATIVE *ptr)
{
	return *ptr;
}

int main(int argc, const char *argv[])
{

	char *rsp = "Y";
	char byte = read_gs((char GS_RELATIVE *)rsp);
	printf("%d\n", byte);
	printf("%d\n", *rsp);
	printf("Might worked\n");
}
