#include <dc/ubc.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define printf(...) \
    do { \
        (printf)(__VA_ARGS__); \
        fflush(stdout); \
    } while(0)

static bool handlerfunc(const ubc_breakpoint_t *bp, 
                        const irq_context_t *ctx,
                        void *ud) {
    printf("HANDLED!!!\n\n");
    return true;
}

static void __no_inline testfunc(void) { 
    ubc_breakpoint_t bpa = {
            .address = (uintptr_t)&&label
        };

    static bool pass = false;

    if(!pass) {
        assert(ubc_enable_breakpoint(&bpa,
                              handlerfunc,
                              NULL));
        pass = true; 
    }

    printf("PRE LABEL!\n\n");

    goto *&&label;

label:    
    printf("HERE!!!!\n");
}



int main(int argc, char* argv[]) {
    #if 0
    ubc_breakpoint_t bpb = {
            .address = (uintptr_t)testfunc
        };

    assert(ubc_enable_breakpoint(&bpb,
                                 handlerfunc,
                                 NULL));
#endif
    testfunc();
    testfunc();
    testfunc();

    return EXIT_SUCCESS;
}