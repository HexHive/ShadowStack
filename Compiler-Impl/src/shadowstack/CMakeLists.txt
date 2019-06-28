add_custom_target(ShadowStack)

set(SHADOWSTACK_SOURCES shadow_stack.c mpxrt-utils.c)

include_directories(..)
find_package (Threads)

#add_compiler_rt_runtime(clang_rt.ShadowStack 
#        SHARED 
#        ARCHS x86_64 
#        SOURCES ${SHADOWSTACK_SOURCES}
#        CFLAGS ${SANITIZER_COMMON_FLAGS}
#        PARENT_TARGET memsafety)

add_compiler_rt_runtime(clang_rt.ShadowStack 
        STATIC 
        ARCHS x86_64 
        SOURCES ${SHADOWSTACK_SOURCES}
        CFLAGS ${SANITIZER_COMMON_FLAGS}
		LINK_LIBS ${CMAKE_THREAD_LIBS_INIT}
        PARENT_TARGET memsafety)

