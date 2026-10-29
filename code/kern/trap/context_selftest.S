#ifdef LAB3_SELFTEST
#include <riscv.h>
    .text
    .align 2
    .macro SNAPSHOT offset
    sd zero, (\offset + 0)(sp)
    sd ra, (\offset + 8)(sp)
    sd sp, (\offset + 16)(sp)
    sd gp, (\offset + 24)(sp)
    sd tp, (\offset + 32)(sp)
    sd t0, (\offset + 40)(sp)
    sd t1, (\offset + 48)(sp)
    sd t2, (\offset + 56)(sp)
    sd s0, (\offset + 64)(sp)
    sd s1, (\offset + 72)(sp)
    sd a0, (\offset + 80)(sp)
    sd a1, (\offset + 88)(sp)
    sd a2, (\offset + 96)(sp)
    sd a3, (\offset + 104)(sp)
    sd a4, (\offset + 112)(sp)
    sd a5, (\offset + 120)(sp)
    sd a6, (\offset + 128)(sp)
    sd a7, (\offset + 136)(sp)
    sd s2, (\offset + 144)(sp)
    sd s3, (\offset + 152)(sp)
    sd s4, (\offset + 160)(sp)
    sd s5, (\offset + 168)(sp)
    sd s6, (\offset + 176)(sp)
    sd s7, (\offset + 184)(sp)
    sd s8, (\offset + 192)(sp)
    sd s9, (\offset + 200)(sp)
    sd s10, (\offset + 208)(sp)
    sd s11, (\offset + 216)(sp)
    sd t3, (\offset + 224)(sp)
    sd t4, (\offset + 232)(sp)
    sd t5, (\offset + 240)(sp)
    sd t6, (\offset + 248)(sp)
    .endm
    .global lab3_context_selftest
lab3_context_selftest:
    addi sp, sp, -640
    sd ra, 512(sp)
    sd s0, 520(sp)
    sd s1, 528(sp)
    sd s2, 536(sp)
    sd s3, 544(sp)
    sd s4, 552(sp)
    sd s5, 560(sp)
    sd s6, 568(sp)
    sd s7, 576(sp)
    sd s8, 584(sp)
    sd s9, 592(sp)
    sd s10, 600(sp)
    sd s11, 608(sp)
    li ra, 257
    li t0, 261
    li t1, 262
    li t2, 263
    li s0, 264
    li s1, 265
    li a0, 266
    li a1, 267
    li a2, 268
    li a3, 269
    li a4, 270
    li a5, 271
    li a6, 272
    li a7, 273
    li s2, 274
    li s3, 275
    li s4, 276
    li s5, 277
    li s6, 278
    li s7, 279
    li s8, 280
    li s9, 281
    li s10, 282
    li s11, 283
    li t3, 284
    li t4, 285
    li t5, 286
    li t6, 287
    SNAPSHOT 0
    .option push
    .option rvc
    c.ebreak
    .option norvc
    ebreak
    .option pop
    SNAPSHOT 256
    li t0, 0
1:
    add t1, sp, t0
    ld t2, 0(t1)
    ld t3, 256(t1)
    bne t2, t3, 2f
    addi t0, t0, 8
    li t1, 256
    bne t0, t1, 1b
    li a0, 0
    j 3f
2:
    li a0, 1
3:
    ld ra, 512(sp)
    ld s0, 520(sp)
    ld s1, 528(sp)
    ld s2, 536(sp)
    ld s3, 544(sp)
    ld s4, 552(sp)
    ld s5, 560(sp)
    ld s6, 568(sp)
    ld s7, 576(sp)
    ld s8, 584(sp)
    ld s9, 592(sp)
    ld s10, 600(sp)
    ld s11, 608(sp)
    addi sp, sp, 640
    ret
#endif
