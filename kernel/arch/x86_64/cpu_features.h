#ifndef SB_ARCH_X86_64_CPU_FEATURES_H
#define SB_ARCH_X86_64_CPU_FEATURES_H

#include <stdint.h>

#define SB_CPUID_EXTENDED_MAX      0x80000000u
#define SB_CPUID_EXTENDED_FEATURES 0x80000001u
#define SB_CPUID_EDX_NX            (1u << 20)
#define SB_MSR_IA32_EFER           0xC0000080u
#define SB_EFER_NXE                (1ull << 11)

static inline void sb_cpu_cpuid(uint32_t leaf,
                                uint32_t subleaf,
                                uint32_t *eax_out,
                                uint32_t *ebx_out,
                                uint32_t *ecx_out,
                                uint32_t *edx_out) {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    __asm__ volatile ("cpuid"
                      : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                      : "a"(leaf), "c"(subleaf));
    if (eax_out != 0) *eax_out = eax;
    if (ebx_out != 0) *ebx_out = ebx;
    if (ecx_out != 0) *ecx_out = ecx;
    if (edx_out != 0) *edx_out = edx;
}

static inline uint64_t sb_cpu_rdmsr(uint32_t msr) {
    uint32_t low;
    uint32_t high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void sb_cpu_wrmsr(uint32_t msr, uint64_t value) {
    const uint32_t low = (uint32_t)value;
    const uint32_t high = (uint32_t)(value >> 32);
    __asm__ volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high) : "memory");
}

/* Ensure that bit 63 in paging entries has architectural NX semantics before
 * any mapping sets it. Without EFER.NXE, an NX PTE is a reserved-bit encoding
 * and user access faults with PFEC.RSVD instead of enforcing execute-disable. */
static inline int sb_cpu_enable_nx(void) {
    static int nx_state;
    if (nx_state > 0) return 0;
    if (nx_state < 0) return -1;

    uint32_t max_extended = 0u;
    sb_cpu_cpuid(SB_CPUID_EXTENDED_MAX, 0u,
                 &max_extended, 0, 0, 0);
    if (max_extended < SB_CPUID_EXTENDED_FEATURES) {
        nx_state = -1;
        return -1;
    }

    uint32_t features_edx = 0u;
    sb_cpu_cpuid(SB_CPUID_EXTENDED_FEATURES, 0u,
                 0, 0, 0, &features_edx);
    if ((features_edx & SB_CPUID_EDX_NX) == 0u) {
        nx_state = -1;
        return -1;
    }

    uint64_t efer = sb_cpu_rdmsr(SB_MSR_IA32_EFER);
    if ((efer & SB_EFER_NXE) == 0u) {
        sb_cpu_wrmsr(SB_MSR_IA32_EFER, efer | SB_EFER_NXE);
        efer = sb_cpu_rdmsr(SB_MSR_IA32_EFER);
    }

    if ((efer & SB_EFER_NXE) == 0u) {
        nx_state = -1;
        return -1;
    }

    nx_state = 1;
    return 0;
}

#endif /* SB_ARCH_X86_64_CPU_FEATURES_H */
