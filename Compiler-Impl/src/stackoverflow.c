#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
typedef void *(*pthread_start_routine) (void *);

pid_t ok_function()
{
    return getppid() & getpid();
}

void the_goal()
{
	execlp("bash", "bash", NULL);
}

void unsafe_function()
{
    //get the base pointer
    void **base = NULL;
    asm("mov %%rbp, %0" : "=r"(base)::);

    //don't try this at home
    //overwrite the return addr
    base[1] = the_goal;
}

int main(int argc, const char *argv[])
{
    printf("About to call a safe function...\n");
    ok_function();
    printf("Retunred from safe function\n");

    printf("About to overwrite the stack...\n");
    if(argc == 2 && strcmp(argv[1], "mt") == 0)
    {
    	pthread_t thread;
	    int tid = pthread_create(&thread, NULL, (pthread_start_routine)unsafe_function, NULL);
	    printf("TID: %d\n", tid);
	    pthread_join(thread, NULL);
    }else
    {
    	unsafe_function();
    }
    printf("Retunred from an unsafe function. This should not happen\n");
    return 0;
}
