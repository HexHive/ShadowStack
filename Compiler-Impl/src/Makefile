all: fib_driver mpk_example pthread_create.so protected unprotected



cc = clang
my = ../debug-build/bin/clang 
CFLAGS = -Wall -g
OPT = -O0
mpk_example: mpk_example.c
	$(cc) mpk_example.c -o mpk_example -Wall

protected: stackoverflow.c 
	$(my) $(CFLAGS) $(OPT) stackoverflow.c -o protected -lpthread
	
unprotected: stackoverflow.c 
	$(cc) $(CFLAGS) $(OPT) stackoverflow.c -o unprotected -lpthread

pthread_create.so: pthread_create.c ShadowStackConstants.h
	$(cc) $(CFLAGS) $(OPT) -g -shared -fPIC pthread_create.c -ldl -o pthread_create.so -lpthread

fib_mpk.o: fib_mpk.c
	$(my) $(CFLAGS) $(OPT) -c fib_mpk.c

fib.o: fib.c
	$(cc) $(CFLAGS) $(OPT) -c fib.c

fib_driver: fib_mpk.o fib.o fib_driver.c
	$(my) $(CFLAGS) $(OPT) fib_driver.c fib.o fib_mpk.o -o fib_driver -lpthread

clean:
	rm -f *.o *.so mpk_example protected unprotected fib fib_mpk fib_driver
