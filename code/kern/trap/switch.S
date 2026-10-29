#ifdef LAB3_CHALLENGE
#include <riscv.h>
    .text
    .align 2
    .global lab3_enter_user
lab3_enter_user:
    # Preserve the kernel C caller's callee-saved registers across the U run.
    addi sp, sp, -112
    sd ra, 0(sp)
    sd s0, 8(sp)
    sd s1, 16(sp)
    sd s2, 24(sp)
    sd s3, 32(sp)
    sd s4, 40(sp)
    sd s5, 48(sp)
    sd s6, 56(sp)
    sd s7, 64(sp)
    sd s8, 72(sp)
    sd s9, 80(sp)
    sd s10, 88(sp)
    sd s11, 96(sp)
    la t0, challenge_resume_sp
    sd sp, 0(t0)
    # Interrupts must stay off between setting sscratch and sret.
    li t0, SSTATUS_SIE | SSTATUS_SPP
    csrc sstatus, t0
    li t0, SSTATUS_SPIE
    csrs sstatus, t0
    csrw sepc, a0
    la t0, user_trap_stack_top
    csrw sscratch, t0
    mv sp, a1
    sret

    .global challenge_kernel_return
challenge_kernel_return:
    ld ra, 0(sp)
    ld s0, 8(sp)
    ld s1, 16(sp)
    ld s2, 24(sp)
    ld s3, 32(sp)
    ld s4, 40(sp)
    ld s5, 48(sp)
    ld s6, 56(sp)
    ld s7, 64(sp)
    ld s8, 72(sp)
    ld s9, 80(sp)
    ld s10, 88(sp)
    ld s11, 96(sp)
    addi sp, sp, 112
    ret

    # Copied into a dedicated PTE_U RX page. No absolute kernel calls or data.
    .option push
    .option norvc
    .align 2
    .global user_payload_start, user_payload_end
    .global user_payload_csr, user_payload_load
user_payload_start:
    addi sp, sp, -16
    li t0, 0x123
    sd t0, 0(sp)
    li a7, 1
    ecall
1:
    li a7, 2
    ecall
    beqz a0, 1b
user_payload_csr:
    csrr t0, sstatus
    li t0, 0xffffffffc0200000
user_payload_load:
    ld t1, 0(t0)
    # Check that the stack still belongs to this user execution.
    ld t0, 0(sp)
    li t1, 0x123
    bne t0, t1, 2f
    li a7, 3
    ecall
2:
    li a7, 99
    ecall
    j 2b
user_payload_end:
    .option pop
#endif
