# LAB1 提示词汇总

> 本文件汇总本实验中使用的原始任务提示词、代码复核提示词、三个分工模块的最终专业提示词，以及测试与报告交付提示词。

````markdown
我们的 Git 仓库：
https://github.com/yuluyuluyulu/os2026-ooo

本次实验为 lab1。分支根目录包含：
- code：实验实现代码；
- report：report.md、prompt.md 和 images 测试截图目录。
````

````markdown
三位成员为：
- 2412930-林永圣
- 2414063-尹天宇
- 2412763-王泽鼎
````

## 代码基线识别提示词

````markdown
[TASK]
只读检查课程提供的 lab1.zip，识别实验类型、源码结构、入口文件、构建目标和
预期运行现象，并与当前工作区版本进行差异分析。

[RELY]
- lab1.zip 是教师实际提供的代码基线。
- 实验报告模板只规定报告结构，不规定代码行为。

[GUARANTEE]
- 不执行压缩包内可能存在的未知脚本。
- 在替换现有 code/ 前先保存可恢复备份。
- 不把其他版本 uCore 的中断、内存或进程模块混入本实验。

[SPECIFICATION]
Pre-Condition：已有一个待复核的 lab1 工作区。
Case 1：若压缩包缺少 trap/clock 等目录，则判定为最小内核实验，重新确定范围。
Post-Condition：输出准确的实验类型、文件清单、差异和后续测试方案。
````

## 构建与镜像生成模块

**负责人：2412930-林永圣**

````markdown
[TASK]
基于课程提供的 lab1.zip，逐条分析 Makefile 中从 C/汇编源文件到
bin/kernel 和 bin/ucore.img 的完整构建流程，并建立可重复的自动测试。

[OPERATION]
1. 检查 WSL 中 riscv64-unknown-elf-gcc、ld、objcopy、objdump、make。
2. 执行 make clean 和 make V=，保留真实命令输出。
3. 使用 file/objdump 验证 ELF 架构、段布局、入口和关键符号。
4. 不修改 kern/init/entry.S、kern/init/init.c 的实验语义。

[RELY]
- Makefile
- tools/function.mk
- tools/kernel.ld
- make V= 的实际输出

[GUARANTEE]
- 解释 -mcmodel=medany、-nostdinc、-fno-builtin、-fno-stack-protector、
  -ffunction-sections、-m elf64lriscv、--gc-sections、-T、--strip-all、
  -O binary 等参数。
- 区分 bin/kernel 的调试用途和 bin/ucore.img 的启动用途。
- 若原 grade 目标依赖缺失脚本，提供范围严格的替代测试，不伪造评分结果。

[SPECIFICATION]
Pre-Condition：工具链在 PATH 中，工作目录为 code/。
Post-Condition：make 成功；make grade 能验证 0x80200000、kern_entry、
bootstacktop、OpenSBI 下一跳和内核输出。
````

## 链接脚本、内存布局与入口模块

**负责人：2414063-尹天宇**

````markdown
[TASK]
结合 tools/kernel.ld、kern/init/entry.S、kern/init/init.c 和最终 ELF，
解释最小内核的装载地址、段布局、内核栈、BSS 初始化及汇编到 C 的衔接。

[OPERATION]
1. 对 kernel.ld 的每个有效语句逐项解释。
2. 分析 la sp, bootstacktop 与 tail kern_init 的指令语义和设计目的。
3. 使用 objdump -h/-t/-d 验证静态地址与反汇编。
4. 使用 GDB 验证进入 kern_init 时 sp 是否等于 bootstacktop。

[RELY]
- BASE_ADDRESS = 0x80200000
- ENTRY(kern_entry)
- .align PGSHIFT、.space KSTACKSIZE
- kern_init 的 noreturn 声明
- edata/end 链接器符号

[GUARANTEE]
- 不臆造源码中不存在的初始化流程。
- 明确 RISC-V 栈向低地址增长。
- 区分 ELF 中的 VMA/LMA、裸镜像和传统 MBR 启动扇区。

[SPECIFICATION]
Post-Condition：给出 kern_entry、kern_init、bootstack、bootstacktop 的实测地址；
证明 sp=bootstacktop；说明 BSS 清零为何是 C 运行环境的一部分。
````

## OpenSBI 启动与 GDB 调试模块

**负责人：2412763-王泽鼎**

````markdown
[TASK]
在 QEMU virt + OpenSBI 环境中启动课程最小内核，分析从复位向量到
kern_entry/kern_init 的控制流，并用 GDB 验证关键状态。

[OPERATION]
1. 原样运行压缩包代码，记录 OpenSBI 的 Firmware Base、Next Address、
   Next Mode 和内核输出。
2. 若只显示 OpenSBI，使用输出证据定位装载参数问题。
3. 启动 QEMU -s -S，使用 RISC-V GDB 观察 0x1000 复位指令。
4. 在 kern_entry 和 kern_init 设置断点，记录 PC、sp 和 bootstacktop。

[RELY]
- QEMU virt 的复位 PC 为 0x1000。
- OpenSBI 位于 0x80000000 并在 M 态运行。
- 内核链接地址和入口为 0x80200000。

[GUARANTEE]
- 区分“QEMU 装载镜像”和“OpenSBI 初始化后跳转内核”。
- 不把本配置描述成 OpenSBI 自行逐扇区读取硬盘。
- 修改 QEMU 参数时同步更新 qemu/debug 目标，并保留 GDB 符号文件。

[SPECIFICATION]
Case 1：Next Address=0 时，比较 -device loader 与 -kernel 行为。
Case 2：专用 GDB 不存在时回退 gdb-multiarch。
Post-Condition：Next Address=0x80200000；出现内核启动信息；
RESET_PC=0x1000；KERN_ENTRY_PC=0x80200000；sp=bootstacktop。
````

## 兼容性问题诊断提示词

````markdown
[TASK]
诊断“lab1 编译成功，但 make qemu 只显示 OpenSBI、没有内核输出”的原因。

[RELY]
- 原命令使用 -device loader,file=bin/ucore.img,addr=0x80200000。
- OpenSBI 输出 Domain0 Next Address = 0x0。
- 内核链接入口为 0x80200000。

[GUARANTEE]
- 先根据日志定位，不修改内核 C/汇编逻辑。
- 修复必须同时适用于 make qemu 和 make debug。
- 修复后检查 Next Address 和内核消息，而不是仅以进程仍在运行为成功。

[SPECIFICATION]
Expected Fix：使用 QEMU -kernel bin/ucore.img，使固件获得下一阶段入口。
Post-Condition：OpenSBI 输出 Next Address=0x80200000，随后出现
(THU.CST) os is loading ...。
````

## 自动测试提示词

````markdown
[TASK]
为最小内核新增不依赖网络、可由 make grade 调用的自动验收脚本。

[RELY]
- 内核设计为永久循环，测试必须使用有限超时。
- timeout 返回 124 表示测试主动结束，不应直接判为失败。

[GUARANTEE]
- 重新执行 clean/build。
- 保存 build、layout、qemu 三类日志。
- 检查 .text/入口地址、kern_entry、bootstacktop、OpenSBI Next Address 和
  内核输出；任一条件缺失时打印日志并非零退出。

[SPECIFICATION]
Success Output：
PASS: custom lab1 acceptance builds, links at 0x80200000, and boots through OpenSBI.

Boundary：该脚本是小组为复现构建、地址和启动现象编写的本地验收工具，
不得将其结果表述为课程官方评分或分数。
````

## 仓库横向复核提示词

````markdown
[TASK]
只读审阅 lab1 分支，检查本组报告是否遗漏
有助于解释课程代码的关键证据或答辩易错点。

[RELY]
- 教师提供的 lab1.zip、模板和本组实测结果始终具有更高优先级。
- 仓库内容只用于横向比较，不保证内容、环境版本或结论正确。

[GUARANTEE]
- 只吸收可由源码、objdump、QEMU 和 GDB 证据独立验证的分析角度。
- 明确两项正式练习：entry.S 指令解释，以及复位到内核入口的 GDB 跟踪。
- 区分构建链与运行链；补充 cprintf 到 SBI ecall 的字符输出链。
- 明确自定义 make grade 只是本地验收，不冒充课程官方评分。

[SPECIFICATION]
Post-Condition：报告结构更清晰、技术边界更准确，所有新增事实均能由本组代码
或实测证据支持，并在参考资料中标明同伴仓库仅供横向比较。
````
