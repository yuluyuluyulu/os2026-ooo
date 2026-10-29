#ifdef LAB3_CHALLENGE
#include <assert.h>
#include <challenge.h>
#include <clock.h>
#include <pmm.h>
#include <riscv.h>
#include <stdio.h>
#include <string.h>

#define USER_BASE 0x40000000UL
#define USER_STACK_TOP (USER_BASE + 2 * PGSIZE)
#define LOAD_PAGE_FAULT 13
#define page2kva(page) ((void *)(page2pa(page) + va_pa_offset))

extern char user_payload_start[], user_payload_end[];
extern char user_payload_csr[], user_payload_load[];
extern char challenge_kernel_return[];
extern void __user_alltraps(void);
extern void lab3_enter_user(uintptr_t entry, uintptr_t stack);

// This stack is never mapped with PTE_U. User sp is never used by C handlers.
char user_trap_stack[8192] __attribute__((aligned(16)));
asm(".global user_trap_stack_top\n"
    "user_trap_stack_top = user_trap_stack + 8192\n");
uintptr_t challenge_resume_sp;
extern uintptr_t *satp_virtual;
static bool active, entered, csr_blocked, kernel_blocked;
static size_t user_ticks;

void challenge_timer_observe(struct trapframe *tf) {
    if (active && !trap_in_kernel(tf)) {
        assert((uintptr_t)tf >= (uintptr_t)user_trap_stack);
        assert((uintptr_t)(tf + 1) <= (uintptr_t)user_trap_stack + sizeof(user_trap_stack));
        ++user_ticks;
        if (user_ticks == 1) {
            cprintf("[Challenge] U timer: SPP=0, kernel trap stack verified\n");
        }
    }
}

bool challenge_exception(struct trapframe *tf) {
    if (!active || trap_in_kernel(tf)) {
        return 0;
    }
    assert((uintptr_t)tf >= (uintptr_t)user_trap_stack);
    assert((uintptr_t)(tf + 1) <= (uintptr_t)user_trap_stack + sizeof(user_trap_stack));
    if (tf->cause == CAUSE_USER_ECALL) {
        switch (tf->gpr.a7) {
        case 1:
            assert(!entered && tf->gpr.sp == USER_STACK_TOP - 16);
            entered = 1;
            cprintf("[Challenge] S -> U: user ecall, SPP=0, user stack verified\n");
            break;
        case 2:
            assert(entered);
            tf->gpr.a0 = user_ticks >= 3;
            break;
        case 3:
            assert(entered && csr_blocked && kernel_blocked && user_ticks >= 3);
            tf->status |= SSTATUS_SPP | SSTATUS_SPIE;
            tf->status &= ~SSTATUS_SIE;
            tf->epc = (uintptr_t)challenge_kernel_return;
            tf->gpr.sp = challenge_resume_sp;
            cprintf("[Challenge] U -> S: ecall restores kernel continuation\n");
            return 1;
        default:
            panic("unexpected user ecall");
        }
        tf->epc += 4;
        return 1;
    }
    uintptr_t csr_pc = USER_BASE + (user_payload_csr - user_payload_start);
    uintptr_t load_pc = USER_BASE + (user_payload_load - user_payload_start);
    if (tf->cause == CAUSE_ILLEGAL_INSTRUCTION && tf->epc == csr_pc) {
        assert(entered && !csr_blocked);
        csr_blocked = 1;
        tf->epc += 4;
        cprintf("[Challenge] U privileged CSR: denied (illegal instruction)\n");
        return 1;
    }
    if (tf->cause == LOAD_PAGE_FAULT && tf->epc == load_pc) {
        assert(entered && !kernel_blocked && tf->badvaddr == KERNBASE);
        kernel_blocked = 1;
        tf->epc += 4;
        cprintf("[Challenge] U read kernel: denied (load page fault)\n");
        return 1;
    }
    print_trapframe(tf);
    panic("unexpected user exception");
}

void lab3_switch_test(void) {
    assert(!active && read_csr(sscratch) == 0);
    size_t free_before = nr_free_pages();
    struct Page *middle = alloc_page(), *leaf = alloc_page();
    struct Page *code = alloc_page(), *stack = alloc_page();
    assert(middle && leaf && code && stack);
    pte_t *l1 = page2kva(middle), *l0 = page2kva(leaf);
    memset(l1, 0, PGSIZE);
    memset(l0, 0, PGSIZE);
    memset(page2kva(stack), 0, PGSIZE);
    assert(user_payload_end - user_payload_start <= PGSIZE);
    memcpy(page2kva(code), user_payload_start, user_payload_end - user_payload_start);
    l1[0] = (page2ppn(leaf) << 10) | PTE_V;
    l0[0] = (page2ppn(code) << 10) | PTE_V | PTE_U | PTE_R | PTE_X | PTE_A;
    l0[1] = (page2ppn(stack) << 10) | PTE_V | PTE_U | PTE_R | PTE_W | PTE_A | PTE_D;
    pte_t *root = (pte_t *)satp_virtual;
    assert(root[1] == 0);
    root[1] = (page2ppn(middle) << 10) | PTE_V;
    asm volatile("sfence.vma\nfence.i" ::: "memory");

    uintptr_t previous_vector = read_csr(stvec);
    active = 1;
    write_csr(stvec, (uintptr_t)__user_alltraps);
    cprintf("[Challenge] user RX=0x40000000, stack RW/NX=0x40001000\n");
    lab3_enter_user(USER_BASE, USER_STACK_TOP);
    // The return instruction itself executes in S mode: reading sstatus works.
    assert((read_csr(sstatus) & SSTATUS_SIE) && read_csr(sscratch) == 0);
    assert(entered && csr_blocked && kernel_blocked && user_ticks >= 3);
    active = 0;
    write_csr(stvec, previous_vector);
    root[1] = 0;
    asm volatile("sfence.vma" ::: "memory");
    free_page(stack);
    free_page(code);
    free_page(leaf);
    free_page(middle);
    assert(nr_free_pages() == free_before);
    cprintf("[Challenge] mode switch: PASS (user timers=%lu, pages restored)\n",
            (unsigned long)user_ticks);
}
#endif
