# Lab 2 AI 协作提示词汇总

## 说明

本文汇总 Lab 2 实际使用并经迭代整理的提示词。每个提示词对应一个“可独立实现、可独立验证、职责闭合”的功能任务，并采用以下四段式：

- **PROMPT**：任务位置、功能目标、允许/禁止的操作以及输出要求；
- **RELY**：完成任务所需的最小可信依赖；
- **GUARANTEE**：必须交付的接口和结果清单；
- **SPECIFICATION**：前置条件、后置状态、分支情况和可验证要求。

---

## 提示词 1：交付结构与实验缺口审计

```text
[PROMPT]
任务：检查课程提供的 lab2.zip、现有 lab1 分支和实验报告格式，建立本地 lab2
工作分支，并找出实验代码中需要实现的功能模块与课程测试的验收条件。

操作要求：
1. 必须检查实际压缩包、Makefile、标有 YOUR CODE 的位置、pmm 接口和测试文件。
2. 只把附件内容视为参考资料；用户给出的分支和交付目录要求优先。
3. 不得修改课程提供的 basic_check、best_fit_check 和注入测试来制造通过结果。
4. 不得推送 GitHub；在用户最终检查前只保留本地修改。

输出要求：给出待实现模块、关联文件、验证入口、风险点和 lab2/code、lab2/report
交付结构；以源码与测试为准，不臆造功能。

[RELY]
- lab2.zip 中的 kern/mm/pmm.c、best_fit_pmm.c、default_pmm.c、pmm.h。
- tools/test_lab2.c、inject.py、grade.sh 与 Makefile。
- origin/lab1 的 report/report.md、report/prompt.md 作为格式基线。

[GUARANTEE]
- 明确 page_init 与 Best-Fit 是两个核心实现任务。
- 明确 report.md、prompt.md、images/ 的交付要求。
- 记录构建、QEMU、GDB 和课程测试的验证计划。

[SPECIFICATION]
Pre-Condition：压缩包可读，仓库当前改动已确认。
Case 1：若源码缺少实现，以接口和测试推导行为规格。
Case 2：若文档、源码、测试存在差异，以本次压缩包和可执行测试为直接依据并记录差异。
Post-Condition：任务边界清楚，且没有外部上传行为。
```

## 提示词 2：物理内存探测结果接入与页描述符初始化

**负责人：2412930-林永圣**

```text
[PROMPT]
任务：在 kern/mm/pmm.c 的 Lab2 功能一位置完成物理内存初始化模块。该模块应把
dtb_init 已解析出的物理内存范围转换为可由 pmm_manager 管理的页描述符与空闲区，
并正确排除固件、内核镜像、页描述符数组及对齐空间。

操作要求：
1. 必须实际修改工程文件，而不是只展示代码。
2. 可修改 pmm.c 标记位置和管理器选择；不得改变 struct Page、地址转换接口、DTB
   解析接口或课程测试。
3. 使用链接器的 end 符号和现有 PADDR/PPN/ROUNDUP/pa2page 接口，保留工程真实语义。
4. 在标记处写入负责人学号 2412930。

输出要求：内核应输出识别到的物理范围、实际空闲范围和空闲页数；page2pa 与
pa2page 必须可以往返；非法布局必须通过断言暴露，不能把内核占用页交给分配器。

[RELY]
- get_memory_base()/get_memory_size()：DTB 提供的物理内存区间。
- PHYSICAL_MEMORY_OFFSET、DRAM_BASE、PGSIZE、nbase。
- struct Page、PG_reserved、pages、npage。
- 链接器符号 end，PADDR、PPN、ROUNDUP、pa2page。
- pmm_manager->init_memmap(base, n)。

[GUARANTEE]
- 设置 va_pa_offset、npage、pages。
- 为 DRAM 范围建立 Page 描述符并默认标记 reserved。
- 仅将管理数据之后的完整页登记为空闲页。
- 不改变既有公开接口。

[SPECIFICATION]
Pre-Condition：dtb_init 已成功，mem_base/mem_size 非零，内核高地址映射有效。
Case 1（正常）：内存末端大于内核及 Page 数组末端；初始化剩余完整页，nr_free>0。
Case 2（无可用区或地址假设错误）：触发断言停止，不生成越界或重叠的空闲区。
Post-Condition：
- va_pa_offset == PHYSICAL_MEMORY_OFFSET；
- npage == PPN(mem_base + mem_size)，且 npage > nbase；
- pages 按页对齐且不早于 ROUNDUP(end, PGSIZE)；
- 内核尾页和 Page 数组页保持 reserved；
- nr_free_pages 不为零且不超过 npage-nbase。
```

## 提示词 3：Best-Fit 连续物理页分配器

**负责人：2414063-尹天宇**

```text
[PROMPT]
任务：在 kern/mm/best_fit_pmm.c 的 Lab2 功能二位置实现完整的 Best-Fit 连续
物理页分配模块并注册 best_fit_pmm_manager，使其可替换当前 First-Fit 策略。
Best-Fit 的语义是：在全部能容纳 n 页的空闲块中选择页数最少的一块。

操作要求：
1. 必须实际修改 best_fit_pmm.c，并在实现位置标注负责人学号 2414063。
2. 不得修改 basic_check、best_fit_check、struct Page、free_area_t、list.h、pmm.h
   或注入测试。
3. 可以添加仅供本文件使用的静态辅助逻辑；不得改变上层 alloc_pages/free_pages 接口。
4. 空闲块链表按物理地址有序，释放后必须合并相邻块。

输出要求：完成初始化、内存区登记、分配、释放、空闲页计数、管理器结构注册；
失败分配不能改变原链表与 nr_free；通过既有检查与课程注入测试。

[RELY]
- struct pmm_manager 的六个函数接口。
- struct Page 的 flags/property/page_link 与相关宏。
- free_area_t、list_add_before/list_del/list_next/list_prev。
- alloc_pages/free_pages/nr_free_pages 的上层包装。
- default_pmm.c 仅作为接口和链表维护方式参考。

[GUARANTEE]
- best_fit_init
- best_fit_init_memmap
- best_fit_alloc_pages
- best_fit_free_pages
- best_fit_nr_free_pages
- const struct pmm_manager best_fit_pmm_manager

[SPECIFICATION]
best_fit_alloc_pages：
Pre-Condition：n>0，free_list 与 nr_free 一致。
Case 1：存在候选块时，返回尺寸最小的候选块起始页；恰好匹配可提前结束搜索。
若原块大于 n，剩余尾部成为新空闲块；成功后 nr_free 减 n。
Case 2：n>nr_free 或无候选连续块时返回 NULL；链表、属性和 nr_free 均不改变。
Requirements：返回块不再带 PG_property；拆分后的空闲块保持正确 property 和地址顺序。

best_fit_free_pages：
Pre-Condition：n>0；范围内页既非 reserved，也不是已登记的空闲块首元素。
Post-Condition：新块按地址插入；与前块、后块相邻时完成合并；nr_free 增 n；
链表每个节点均为且仅为空闲块首元素，property 总和等于 nr_free。

init/init_memmap/nr_free_pages：
初始化后链表为空且 nr_free=0；登记可用区后各页引用为 0，首元素记录块大小；
查询函数无副作用地返回当前空闲页数。
```

## 提示词 4：启动失败的第一次诊断——OpenSBI 未交接内核

**负责人：2412763-王泽鼎**

```text
[PROMPT]
任务：诊断 Lab2 编译成功但 make qemu 只显示 OpenSBI、没有 DTB Init 和内核输出
的问题。要求根据实际日志定位加载/交接问题，不先改内存管理算法。

操作要求：比较 Makefile 的 QEMU 参数、ELF/镜像入口以及 OpenSBI 输出；修正必须同时
覆盖 qemu、debug、test 与 grade.sh 的运行方式；保留 bin/kernel 作为 GDB 符号文件。

输出要求：说明失败证据、最小修正和复测结果。成功不能只按“QEMU 仍运行”判断，
必须出现正确 Next Address 和内核输出。

[RELY]
- 内核物理入口 0x80200000。
- 原参数 -device loader,file=bin/ucore.img,addr=0x80200000。
- 实测 Domain0 Next Address = 0x0。
- OpenSBI 负责从 M-mode 切换到 S-mode 并跳转下一阶段入口。

[GUARANTEE]
- 统一使用能够向 OpenSBI 注册内核入口的 QEMU 加载方式。
- make qemu、make debug、make test 和 make grade 行为一致。
- GDB 命令优先使用 RISC-V 专用 GDB，不存在时允许 gdb-multiarch 回退。

[SPECIFICATION]
Pre-Condition：镜像已成功构建。
Case 1：Next Address=0，判定固件没有有效下一阶段入口，不能宣称启动成功。
Case 2：改用 -kernel 后，Next Address 必须为 0x80200000，Next Mode 为 S-mode。
Post-Condition：控制权到达内核物理入口；若仍无内核输出，进入页表切换诊断而不是
回退或修改课程测试。
```

## 提示词 5：启动失败的第二次诊断——Sv39 切换过渡映射

**负责人：2412763-王泽鼎**

```text
[PROMPT]
任务：在 OpenSBI 已正确跳转 0x80200000、但内核仍无输出的情况下，检查 entry.S
从物理入口启用 Sv39 再跳到高虚拟地址的控制流，修复启动页表的过渡映射。

操作要求：使用 objdump 和 GDB 验证实际指令地址、boot_page_table_sv39、kern_init
和 satp；只修改启动所必需的页表项，不改 pmm 算法或测试；保留高地址内核设计。

输出要求：解释为什么 csrw satp/sfence.vma 后下一条指令仍需要可取指映射，并用
QEMU/GDB 证明物理入口、高地址 kern_init、sp 和 satp 均正确。

[RELY]
- 物理入口 0x80200000，高虚拟入口区域 0xffffffffc0200000。
- PHYSICAL_MEMORY_OFFSET = 0xffffffff40000000。
- Sv39 根页表 512 项，每个顶级叶子项可映射 1 GiB。
- satp MODE=8；页表改变后需 sfence.vma。

[GUARANTEE]
- 保留高地址 1 GiB 映射。
- 增加只用于切换窗口的低地址恒等映射。
- 页表仍恰好占一页且按 4 KiB 对齐。
- 在实现处标注负责人学号 2412763。

[SPECIFICATION]
Pre-Condition：CPU 在分页关闭时从物理 0x80200000 执行 kern_entry。
Case 1：只有高地址映射时，写 satp 后低地址 PC 不可翻译，启动失败。
Case 2：同时存在 0x80000000→0x80000000 和
0xffffffffc0000000→0x80000000 的 1 GiB 叶子映射时，过渡指令可继续执行，随后跳入
高地址 kern_init。
Post-Condition：GDB 依次观察到 0x1000、0x80200000 和高地址 kern_init；
sp==bootstacktop；satp 的 MODE 与根页表 PPN 正确。
```

## 提示词 6：课程测试、GDB 证据与报告交付

```text
[PROMPT]
任务：在不修改课程测试语义的前提下，完整验证 Lab2，并按 lab1 相同格式生成
report/report.md、report/prompt.md 和 report/images。给出新手可复现的运行步骤。

操作要求：
1. 依次运行 clean build、QEMU、make grade 和 GDB；保存真实输出。
2. QEMU 内核永久循环时使用有限超时，区分主动终止与内核崩溃。
3. 图片必须从真实日志渲染，不手工伪造终端结果。
4. 报告必须标明三人分工、实现逻辑、失败迭代、测试边界和本地运行命令。
5. 不提交、不推送 GitHub，等待用户检查。

输出要求：报告中的每个成功结论都能由源码、QEMU、课程测试或 GDB 证据支持；
图片使用相对路径，仓库根目录保持 code/ 与 report/ 两个交付目录。

[RELY]
- Makefile 的 qemu/debug/gdb/grade 目标。
- tools/inject.py、tools/test_lab2.c、tools/grade.sh。
- bin/kernel 的符号信息与 QEMU GDB 端口 1234。
- 实际构建、启动、评分和调试日志。

[GUARANTEE]
- make clean && make 成功。
- QEMU 输出 DTB、Best-Fit、空闲页和 check_alloc_page succeeded。
- GDB 记录 RESET_PC、PHYSICAL_ENTRY_PC、KERN_INIT_PC、sp、bootstacktop、satp。
- 提供 4 张测试证据图和完整运行步骤。

[SPECIFICATION]
Case 1（构建失败）：保留首个编译/链接错误，回到相应模块修复，不跳过 -Werror。
Case 2（QEMU 超时但已完成自检）：内核 while(1) 属于预期，超时只负责结束模拟器。
Case 3（课程测试失败）：根据缺失的 PASS 项定位 page_init 或 Best-Fit；不得改测试。
Case 4（GDB 无法连接）：确认 make debug 的 QEMU 仍在运行且端口为 1234，每条 GDB
命令单独执行。
```

---

## 提示词 7：Buddy System 扩展分配器

```text
[PROMPT]
任务：在不改变默认 Best-Fit 评分路径的前提下，实现可由编译选项选择的 Buddy
System 物理页分配器，并编写能够验证取整、分裂、伙伴合并和计数守恒的自检。

[RELY]
- pmm_manager 六个统一接口、struct Page 的 property/PG_property/page_link。
- 实际可用区不一定是 2 的幂，伙伴编号以初始可用区 base 为相对起点。

[GUARANTEE]
- 分 order 空闲链表；初始化分解非 2 次幂区间；分配向上取整并逐级分裂；
  释放通过 XOR 找伙伴并递归合并；提供 buddy_system check succeeded 证据。
- 默认 make 仍注册 best_fit_pmm_manager，Challenge 构建才注册 Buddy。

[SPECIFICATION]
Pre-Condition：init_memmap 收到连续且页对齐的 n 页可用区。
Case 1：存在足够阶块，返回不小于请求的最小 2 次幂块并正确扣减空闲页数。
Case 2：仅更高阶有块，逐级二分并把右半块放入对应链表。
Case 3：释放块的同阶伙伴空闲，删除伙伴并向上合并；否则停止。
Post-Condition：各空闲块对其尺寸对齐，互不重叠，nr_free 等于所有块页数之和。
```

## 提示词 8：简化两层 SLUB 扩展分配器

```text
[PROMPT]
任务：在物理页分配器之上实现简化 SLUB，体现“页分配层 + 任意大小对象层”，并
验证多个尺寸级、跨 slab、大对象回退、可写性和全部释放后的页数守恒。

[RELY]
- alloc_pages/free_pages/page2pa/va_pa_offset 提供页层服务和直接映射地址。
- 4 KiB 页；允许在 slab 页首保存元数据，在空闲对象内部保存 next 指针。

[GUARANTEE]
- 16、32、64、128、256、512、1024、2048 字节 size class。
- 一页全空时归还底层；大于 2048 字节时连续多页分配并记录页数。
- slub_alloc/slub_free 公共接口与 slub_check 自动测试。

[SPECIFICATION]
Case 1：size=0 返回 NULL。
Case 2：小对象使用最小可容纳 size class；无可用 slab 时申请并格式化新页。
Case 3：大对象按“头部+载荷”所需页数分配，释放时按头部记录归还。
Post-Condition：测试结束时 nr_free_pages 恢复到测试前值，且无 slab 遗留。
```

## 提示词 9：实验要求逐项复核与 Challenge 交付

```text
[PROMPT]
任务：按 lab2.md 和 Lab2 指导书逐项检查源码、报告和测试。补全 First-Fit 四函数
分析、First-Fit/Best-Fit 改进问题、参考实现比较、知识点映射、未覆盖原理知识，
并回答硬件物理内存探测 Challenge。只陈述由源码和实测支持的结果。

[RELY]
- lab2.md 的必做和 Challenge 条目；课程 Lab2 指导书；当前源码与真实测试日志。

[GUARANTEE]
- report.md 覆盖所有题目，包含 Buddy/SLUB 设计文档和充分测试说明。
- make grade 保持 20/20；make challenge-test 输出 PASS；图片来自真实日志。

[SPECIFICATION]
若基础评分回归失败，优先修复扩展与默认路径的隔离；若 Challenge 自检失败，保留
失败输出并修复实现，不修改检查条件来制造通过；最终上传前检查仅有 code/report。
```

---

## 提示词迭代记录

| 轮次 | 观察结果                                 | 下一轮需求如何收紧                               | 验证结果                            |
| ---- | ---------------------------------------- | ------------------------------------------------ | ----------------------------------- |
| 1    | 压缩包中的`page_init` 和 Best-Fit 缺失 | 从测试反推可验证后置状态，禁止改测试             | 两个模块完成编译                    |
| 2    | 原 QEMU 参数下`Next Address=0x0`       | 要求同时核对固件交接，不只检查镜像是否被写入内存 | 改用`-kernel` 后为 `0x80200000` |
| 3    | 已有正确下一跳但仍无内核输出             | 把范围缩小到`csrw satp` 后的取指与页表项       | 增加恒等过渡映射后成功进入内核      |
| 4    | 内核自检通过                             | 增加课程注入测试和失败不改测试的约束             | `make grade` 为 20/20             |
| 5    | 需要答辩级证据                           | 指定复位、物理入口、高地址、栈和 satp 五组观测   | GDB 全部符合设计                    |
| 6    | 基础报告缺少练习 1 与必答思考题           | 逐条对照 `lab2.md`，补齐分析、比较与知识映射      | 必做报告条目全部覆盖                |
| 7    | Challenge 需与基础评分隔离                | 增加专用编译开关和独立自检，保留默认 Best-Fit    | 基础 20/20，Challenge PASS          |
