#ifndef __MPK
#define __MPK
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <errno.h>

//these constatns are not defined in sys/mman.h, hardcode them
//PKRU[2i] is the access-disable bit for protection key i (ADi);
//PKRU[2i+1] is the write-disable bit
//for protection key i (WDi)
//checked on intel's manual, section 4.6.2, protection keys
#define PKEY_DISABLE_ACCESS 0x1
#define PKEY_DISABLE_WRITE  0x2

#define errExit(msg)    do { perror(msg); exit(errno); \
} while (0)

__attribute__((always_inline))
static inline int
wrpkru(unsigned int pkru)
{
    unsigned int eax = pkru;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    
    asm volatile(".byte 0x0f,0x01,0xef\n\t"
                 : : "a" (eax), "c" (ecx), "d" (edx));
    return 0;
}

__attribute__((always_inline))
static inline int
pkey_set(int pkey, unsigned long rights, unsigned long flags)
{
    unsigned int pkru = (rights << (pkey << 1));
    return wrpkru(pkru);
}

__attribute__((always_inline))
static inline int
pkey_mprotect(void *ptr, size_t size, unsigned long orig_prot,
              unsigned long pkey)
{
    return syscall(SYS_pkey_mprotect, ptr, size, orig_prot, pkey);
}

__attribute__((always_inline))
static inline int
pkey_alloc(void)
{
    return syscall(SYS_pkey_alloc, 0, 0);
}

__attribute__((always_inline))
static inline int
pkey_free(unsigned long pkey)
{
    return syscall(SYS_pkey_free, pkey);
}

#endif
