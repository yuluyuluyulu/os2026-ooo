# Lab 3 AI 协作提示词汇总

| 成员 | 负责内容 | 对应提示词 |
|------|----------|------------|
| 2412930-林永圣 | Lab2 依赖迁移、Trap 上下文恢复与汇编入口 | 提示词 1～2 |
| 2414063-尹天宇 | 中断、异常和时钟关机 | 提示词 3 |
| 2412763-王泽鼎 | 独立自检、GDB 验证与证据整理 | 提示词 4～5 |

---

## 提示词 1：迁移 Lab2 依赖并保留中断保护

**负责人：2412930-林永圣**

```text
[PROMPT]
任务：以本次 lab3.zip 为基础，将上次实验的 page_init 和 Best-Fit 实现接入 Lab3。
只在 pmm.c 的 Lab2 依赖位置填入初始化逻辑，并注册 best_fit_pmm_manager。
不要用 Lab2 的整个 pmm.c 覆盖 Lab3，因为新框架的内存分配包装已经加入中断保护。

操作要求：
1. 复用 DTB、Page、页号换算、保留区和空闲区登记接口，在实现处保留学号 2412930。
2. 保留 alloc_pages/free_pages/nr_free_pages 中 local_intr_save/local_intr_restore。
3. 保留 Lab3 的 trap、clock、intr 初始化顺序和课程测试注入锚点。
4. 检查本机 QEMU/OpenSBI 是否需要 -kernel，以及开启分页时的临时恒等映射。
5. 不删除或降低原分配器自检；不把旧实验截图当作 Lab3 证据。

[RELY]
- 上次已实现的 page_init 与 best_fit_pmm_manager。
- 本次 Lab3 的 Page/pmm_manager、DTB、PADDR/PPN/ROUNDUP/pa2page。
- kern/sync/sync.h 与 kern/driver/intr.h。
- 链接器 end、PHYSICAL_MEMORY_OFFSET、nbase、PGSIZE。

[GUARANTEE]
- pages/npage/va_pa_offset 初始化正确，固件、内核和元数据不进入空闲表。
- 默认使用 Best-Fit，能够通过 check_alloc_page。
- 临界区操作结束后恢复原 SIE，而不是无条件开中断。
- 不修改课程评分断言和分值；启动参数适配必须说明原因。

[SPECIFICATION]
Pre-Condition：有效的单段 QEMU DRAM，内核高地址映射和 DTB 可用。
Case 1：正常初始化，只登记管理数组之后的完整可用页。
Case 2：非法内存布局，明确断言停止，不继续登记错误页面。
Case 3：调用前 SIE=0，分配与释放后仍为 0。
Case 4：调用前 SIE=1，临界区屏蔽中断，完成后恢复为 1。
Post-Condition：内存自检通过，空闲页计数守恒，中断包装仍来自 Lab3 框架。
```

## 提示词 2：Trap 上下文恢复与汇编入口

**负责人：2412930-林永圣**

```text
[PROMPT]
任务：填写 kern/trap/trapentry.S 中两处 lab3 function1，完成 RESTORE_ALL 和
__alltraps 到 trap(struct trapframe *tf) 的调用。在标记处使用学号 2412930。
先阅读 SAVE_ALL 与 trap.h，按实际布局恢复，不根据印象编写偏移。

操作要求：
1. 不改变 SAVE_ALL、struct trapframe 和 trap() 签名。
2. RV64 下 frame 共 36 个 8 字节槽，CSR 槽为 32～35。
3. 恢复 sstatus 和 sepc；用于搬运 CSR 的临时寄存器也要恢复原值。
4. 保存的 x2 是进入 Trap 前的 sp，必须最后从槽位 2 恢复。
5. 按 ABI 用 a0 传递 frame 地址；C 调用结束后 sp 仍须指向同一 frame。
6. 保留 sret。若工具链拒绝旧 CSR 名称，可增加等价别名，不能改变 frame 布局。

[RELY]
- 现有 SAVE_ALL、RESTORE_ALL、__alltraps、__trapret。
- struct pushregs/struct trapframe 与 REGBYTES/LOAD/STORE。
- RISC-V ABI、sstatus/sepc/sscratch 和 sret 语义。

[GUARANTEE]
- 全部可写通用寄存器正确恢复，x0 保持硬件零值。
- sret 使用 C 处理后 frame 中的 epc/status。
- 不在仍需读 frame 时提前恢复 sp。
- 汇编与 C 层能形成多次进入、处理、恢复和返回的闭环。

[SPECIFICATION]
Pre-Condition：基础路径来自 S 态，内核栈可写且按 ABI 对齐，SAVE_ALL 已保存完整现场。
Case 1：处理函数没有改变某个 GPR 槽，返回后该寄存器与进入前相同。
Case 2：处理函数修改 epc，返回后执行新的地址。
Case 3：时钟中断未修改 epc，返回被打断的位置并能再次接收中断。
Post-Condition：原 sp 恢复，frame 布局不变，控制权通过 sret 回到保存状态指定的级别。
```

## 提示词 3：中断、异常、时钟统计与关机

**负责人：2414063-尹天宇**

```text
[PROMPT]
任务：在 kern/trap/trap.c 的 lab3 function2 标记处实现 interrupt_handler 和
exception_handler，学号标注为 2414063。保持框架的分发、打印及其他函数语义。

操作要求：
1. 中断原因清除最高位再分类；IRQ_S_TIMER 每次重新设置事件并只增加一次 ticks。
2. 每 TICK_NUM=100 次输出框架规定的 100 ticks。
3. 包中未定义 SHUTDOWN_AFTER，补充可覆盖的默认值 10，并在报告注明选择依据。
4. 达到 10 次统计后调用 libs/sbi.h 的 sbi_shutdown，不以死循环代替关机。
5. 其他已识别中断打印类型，未知中断输出原因及完整 frame。
6. 非法指令输出 Illegal instruction at，断点输出 Breakpoint at。
7. 根据指令编码区分本次目标的 16/32 位长度，禁止统一 epc+=4。
8. 未要求恢复的异常打印 frame；不得伪造恢复成功或修改课程测试。

[RELY]
- struct trapframe、trap_dispatch、print_trapframe、trap_in_kernel。
- IRQ_*、CAUSE_ILLEGAL_INSTRUCTION、CAUSE_BREAKPOINT。
- clock_set_next_event、volatile ticks、TICK_NUM、sbi_shutdown。
- 本实验可读取的 S 态 16/32 位指令编码。

[GUARANTEE]
- 时钟事件持续设置，ticks 单调按实际事件增加；时钟不跳过正常指令。
- 非法指令与断点输出符合课程测试，返回后不重复执行原异常指令。
- 16 位指令 epc 增加 2，32 位增加 4。
- 默认累计 1000 ticks 后经 SBI 退出 QEMU。
- 原有公共接口、打印函数和评分条件不变。

[SPECIFICATION]
Case 1：S timer，设置下次事件，ticks++；到 100 的整数倍输出统计。
Case 2：第 10 次统计，输出累计值并请求关机。
Case 3：c.ebreak 与普通 ebreak，分别跳过 2 与 4 字节。
Case 4：受控非法 16/32 位指令，打印后跳过其长度。
Case 5：未知原因，输出诊断信息，不宣称已实现通用故障修复。
Post-Condition：课程评分三项通过，独立测试还能观察实际关机。
```

## 提示词 4：独立 Trap 与中断状态自检

**负责人：2412763-王泽鼎**

```text
[PROMPT]
任务：在独立文件中补充真实运行测试，不改随包 test_lab3 测试、评分条件或期望值。
测试应通过实际 ebreak/非法指令进入硬件 Trap，而不是只手动调用处理函数。
提供 make selftest，保存可复核日志；默认构建不启用该自检宏。

操作要求：
1. 汇编测试先保存 C 调用者的 ra 与 s0～s11，测试完必须恢复，遵守 ABI。
2. 在 16 位 c.ebreak 与显式 32 位 ebreak 前后保存全部 32 个 GPR 槽逐项比较。
3. gp/tp 保留原值并参与比较，sp/ra/a0 也不得遗漏。
4. 对 16 位保留非法编码和 32 位非法编码，在返回后检查计算结果。
5. 分别测试 SIE=0/1 下的页分配包装，核对状态与页数守恒。
6. QEMU 必须真正退出；超时、panic、非零状态或统计次数不符都判失败。

[RELY]
- 完成后的 Trap 入口与处理器。
- LAB3_SELFTEST 编译开关、alloc_page/free_page/nr_free_pages。
- 课程工具链、Python subprocess、QEMU 的真实串口输出与退出码。

[GUARANTEE]
- 保留课程测试，新增检查能验证课程测试以外的关键边界。
- 32 槽比较失败、EPC 错误或 SIE 错误都产生明确失败。
- 自动日志写入 report/evidence，不生成伪运行截图。

[SPECIFICATION]
Case 1：两种断点恢复后，寄存器快照完全一致。
Case 2：两种非法指令之后的计算结果为 42，证明从正确边界继续执行。
Case 3：原先关闭中断则保持关闭，原先开启则恢复开启，分配释放页数守恒。
Case 4：恰好 10 行 100 ticks，1000 ticks 后正常退出。
Case 5：22 秒未退出，记录输出并失败，不能吞掉超时当作成功。
Post-Condition：自检输出三项 PASS，运行器验证实际结束条件。
```

## 提示词 5：GDB 验证与证据整理

**负责人：2412763-王泽鼎**

```text
[PROMPT]
任务：为 Trap 上下文恢复与异常返回提供 GDB 验证和可复现的截图步骤。
报告只引用实际编译、运行和调试结果，保持本地未提交状态。

操作要求：
1. 提供 make gdb-evidence，使用本次带 LAB3_SELFTEST 的 bin/kernel 连接暂停的 QEMU。
2. 分别停在 C 入口、__trapret 和 sret 后的 EPC，打印 frame、cause、status 和 sp。
3. 在压缩断点处检查指令首半字，核对处理前后的 EPC 差值为 2。
4. 优化构建下使用明确汇编边界的硬件临时断点，不依赖 finish 猜测返回位置。
5. 比较 sret 后的 sp 与 frame 保存的原 sp，检查 sscratch 为 0。
6. 提供手动双终端操作步骤，命令逐条执行，说明退出方式和构建宏的影响。
7. 整理课程评分、独立自检、默认关机和 GDB 证据，不修改评分条件或编造截图。
8. 保持公共接口和课程测试不变，不执行 git add、git commit 或 git push。

[RELY]
- 已实现的 Trap 入口、恢复宏、中断与异常处理器。
- 独立自检、课程评分脚本、QEMU 调试接口与 RISC-V GDB。
- struct trapframe、tf->epc、tf->gpr.sp、__trapret 和 sret。
- report/report.md、report/prompt.md 和 report/images 的组织形式。

[GUARANTEE]
- frame 布局、异常原因与返回地址均能直接在调试器中观察。
- GDB 展示压缩断点的 EPC 增量为 2，以及 sret 后的栈恢复。
- 实际运行截图对应构建、默认时钟关机、课程评分、自检和手动 GDB 五组结果。
- 报告区分测试目标与实际证据，明确当前验证范围。

[SPECIFICATION]
Pre-Condition：QEMU 以 -S 暂停，GDB 加载与客体镜像对应的 ELF。
Case 1：C 入口处 frame 大小为 288，cause=3，指令首半字为 0x9002。
Case 2：到达 __trapret 时，处理后的 EPC 比原值增加 2。
Case 3：到达返回 EPC 时，PC 等于更新后的 EPC，sp 等于保存的原 sp，sscratch=0。
Case 4：课程评分为 100/100，自检与默认关机结果分别由对应输出验证。
Case 5：调试命令输入错误应如实定位并重新输入，不据此判断内核恢复失败。
Post-Condition：证据链接有效、图片编号连续，材料留在本地供小组复核。
```
