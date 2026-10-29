#ifdef LAB3_SELFTEST
#include <assert.h>
#include <clock.h>
#include <intr.h>
#include <pmm.h>
#include <riscv.h>
#include <stdio.h>
#include <trap.h>

extern int lab3_context_selftest(void);

void lab3_selftest(void) {
    assert(sizeof(struct trapframe) == 288);
    assert(offsetof(struct trapframe, status) == 32 * sizeof(uintptr_t));
    assert(offsetof(struct trapframe, epc) == 33 * sizeof(uintptr_t));
    assert((read_csr(sstatus) & SSTATUS_SIE) == 0);
    assert(lab3_context_selftest() == 0);
    cprintf("[Selftest] all 32 GPR slots, 16/32-bit breakpoints: PASS\n");
    // Compressed reserved instruction (2 bytes), followed immediately by a
    // 4-byte instruction. A fixed +4 would resume in its middle and fail.
    uintptr_t value = 0;
    asm volatile(".option push\n.option norvc\n"
                 ".2byte 0x0000\naddi %0, zero, 17\n"
                 ".word 0x0000007b\naddi %0, %0, 25\n.option pop"
                 : "+r"(value) :: "memory");
    assert(value == 42);
    cprintf("[Selftest] 16/32-bit illegal instruction EPC: PASS\n");

    // Verify the Lab3 interrupt-masking wrappers preserve the caller's SIE.
    struct Page *page = alloc_page();
    assert(page && !(read_csr(sstatus) & SSTATUS_SIE));
    free_page(page);
    intr_enable();
    size_t before = nr_free_pages();
    page = alloc_page();
    assert(page && (read_csr(sstatus) & SSTATUS_SIE));
    free_page(page);
    assert(nr_free_pages() == before && (read_csr(sstatus) & SSTATUS_SIE));
    intr_disable();
    assert(ticks == 0);
    cprintf("[Selftest] allocator SIE preservation and page conservation: PASS\n");
}
#endif
