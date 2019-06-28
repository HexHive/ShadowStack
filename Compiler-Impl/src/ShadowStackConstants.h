#define SHADOW_STACK_OFFSET (-(1<<30))
#define PKEY 1
#define NO_PROTECTION 0
#define WRITE_PROTECTION 8
#define PATCH_CALL
#define SHADOW_STACK_PTR ((char*)0x7ffff690d000)
// #define SHADOW_STACK_REG_CLAVE ((char*)0x7ffff7fc0000)
#define SHADOW_STACK_SENTINEL ((char*)0x40000000U)
#define MAX_ADDR 0xFFFFFFFFFFFFFFFFL
#define PRACTICAL_STACK_SIZE 0x800000L

