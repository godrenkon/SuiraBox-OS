#include <assert.h>
#include <stdint.h>
#include "../kernel/arch/x86_64/gdt.h"

char stack_top;

int main(void) {
    assert(!gdt_kernel_stack_pointer_valid(0u));
    assert(gdt_kernel_stack_pointer_valid(16u));
    assert(gdt_kernel_stack_pointer_valid(0x1000u));
    assert(!gdt_kernel_stack_pointer_valid(1u));
    assert(!gdt_kernel_stack_pointer_valid(0x1008u));
    return 0;
}
