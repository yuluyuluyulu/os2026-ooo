# Lab 2 AI 协作提示词汇总

| 成员 | 负责内容 | 对应提示词 |
|---|---|---|
| 2412930-林永圣 | 物理内存探测结果接入与 `page_init` | 提示词 1 |
| 2414063-尹天宇 | Best-Fit、Buddy System 及 Buddy 测试 | 提示词 2～4 |
| 2412763-王泽鼎 | SLUB 实现及 SLUB 测试 | 提示词 5～6 |

---

## 提示词 1：物理内存探测结果接入与页描述符初始化

**负责人：2412930-林永圣**

```text
[PROMPT]
任务：在 kern/mm/pmm.c 的 Lab2 功能一位置实现 static void page_init(void)。该函数
应把 dtb_init 已解析出的物理内存范围转换为可由 pmm_manager 管理的 Page 描述符
和初始空闲区，并正确排除 OpenSBI、内核镜像、Page 描述符数组及页对齐占用。

操作要求：
1. 必须实际修改工程文件，而不是只展示代码。
2. 可修改 pmm.c 标记位置和管理器选择；不得改变 struct Page、地址转换接口、DTB
   解析接口或课程测试。
3. 使用链接器符号 end 和现有 PADDR/PPN/ROUNDUP/ROUNDDOWN/pa2page 接口，
   不自行发明与工程现有定义冲突的地址换算公式。
4. 在标记处写入负责人学号 2412930。
5. 必须明确区分虚拟地址 VA、物理地址 PA、物理页号 PPN 和 Page 描述符地址。
6. 对 mem_base + mem_size 做溢出和上界检查；空闲区首地址向上对齐、末地址向下对齐。

输出要求：内核应输出识别到的物理范围、实际空闲范围和空闲页数；page2pa 与
pa2page 必须可以往返；无效 DTB 或非法内存布局应明确 panic/停止初始化，不能静默
使用伪造范围；若对齐后不存在完整空闲页，则不得调用 init_memmap。

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
Case 2（DTB 无效）：停止物理内存初始化并报告原因，不使用默认范围冒充探测结果。
Case 3（地址溢出或超出内核可管理上界）：截断到合法上界或明确报错，禁止回绕。
Case 4（对齐后无完整空闲页）：不调用 init_memmap，不生成越界或重叠的空闲区。
Post-Condition：
- va_pa_offset == PHYSICAL_MEMORY_OFFSET；
- npage 与合法、可管理的物理末地址一致，且不小于 nbase；
- pages 按页对齐且不早于 ROUNDUP(end, PGSIZE)；
- 内核尾页和 Page 数组页保持 reserved；
- 正常内存布局下 nr_free_pages 不为零，且不超过 npage-nbase。
```

## 提示词 2：Best-Fit 连续物理页分配器

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

## 提示词 3：Buddy System 模块实现与内核接入

**负责人：2414063-尹天宇**

```text
[PROMPT]
任务：在 kern/mm/buddy_pmm.c 中实现 Buddy System 物理页分配器，并通过现有
pmm_manager 接口接入内核。默认课程评分仍使用 Best-Fit；仅在 Challenge 编译选项
启用时切换到 Buddy，不能破坏基础实验路径。

操作要求：
1. 必须实现 init、init_memmap、alloc_pages、free_pages、nr_free_pages 和 check 接口。
2. 按 order 维护空闲块；第 k 阶块包含 2^k 个连续物理页。
3. 实际空闲区的基址和页数不一定是 2 的幂，伙伴编号必须以当前管理区 base 为相对起点。
4. 不修改 struct Page、pmm_manager 公共接口、基础测试或 Best-Fit 默认实现。
5. 对请求页数向上取整到最小 2 次幂；不得把内部取整后的页数误计为用户原请求大小。
6. 在代码中写清分裂、伙伴定位、合并及非 2 次幂初始化的原因。

[RELY]
- pmm_manager 六个统一接口。
- struct Page 的 property、PG_property、page_link 和地址换算接口。
- list.h 的双向链表操作。
- alloc_pages/free_pages 的上层包装与当前可用物理区间。

[GUARANTEE]
- 分 order 的空闲链表和准确的 nr_free 计数。
- 初始化时能把任意长度连续区间分解为对齐的 2 次幂块。
- 分配时找到最小可用阶，并把更高阶块逐级二分。
- 释放时使用相对页号计算伙伴，仅在同阶、同一管理区且伙伴空闲时递归合并。
- 默认 make 注册 best_fit_pmm_manager，Challenge 构建才注册 Buddy。

[SPECIFICATION]
Pre-Condition：init_memmap 收到连续、页对齐且不与保留页重叠的 n 页可用区。
Case 1：目标阶有空闲块，直接取出一块。
Case 2：只有更高阶有块，逐级二分，每次将未使用的另一半放入低一阶链表。
Case 3：所有可用阶都无块，返回 NULL，链表和 nr_free 不变。
Case 4：释放块的同阶伙伴空闲，删除伙伴并合并到上一阶；否则在当前阶停止。
Post-Condition：所有空闲块按自身大小对齐、互不重叠；各链表块大小之和等于 nr_free。
```

## 提示词 4：Buddy System 测试设计与生成

**负责人：2414063-尹天宇**

```text
[PROMPT]
任务：只为已经实现的 Buddy System 设计和补充自动测试，不重写分配器，不降低断言，
不通过修改期望结果制造成功。测试应验证 Buddy 的核心行为、边界情况和状态守恒，
并能够接入 make challenge-test 输出清晰的通过或失败信息。

操作要求：
1. 测试前记录 nr_free_pages，测试结束释放全部对象并检查页数恢复。
2. 每个用例均检查返回值、块大小/对齐、空闲页变化和必要的数据可写性。
3. 测试失败时指出具体阶段，不允许只输出一个无原因的 Error 1。
4. 不依赖固定 128 MiB 或某个固定 Page 地址，测试应适应实际 DTB 内存范围。

[RELY]
- buddy_pmm_manager 的公开接口和 Challenge 选择开关。
- alloc_pages、free_pages、nr_free_pages。
- QEMU 控制台输出、Makefile 的 challenge-test 目标。

[GUARANTEE]
测试至少覆盖：
- 申请 1 页和恰好为 2 次幂的页数；
- 非 2 次幂请求的向上取整；
- 从高阶块连续分裂到目标阶；
- 释放两个伙伴后向上合并；
- 非伙伴相邻块不能错误合并；
- 超过可用内存的申请返回 NULL 且状态不变；
- 多次交错申请/释放后无重叠，最终空闲页数恢复。

[SPECIFICATION]
Case 1（成功分配）：返回块满足目标阶对齐，不与仍存活块重叠。
Case 2（失败分配）：返回 NULL，所有空闲链表及 nr_free 保持不变。
Case 3（合并）：只有地址满足伙伴关系且 order 相同的块才能合并。
Case 4（清理）：无论中间测试顺序如何，正常完成后所有测试块均被归还。
Post-Condition：控制台输出 buddy_system check succeeded，且测试前后 nr_free_pages 相等。
```

### Buddy 测试覆盖说明

上述测试把伙伴系统拆成“取整—查找—分裂—使用—释放—合并”六个阶段。1 页、2 的幂和非 2 的幂请求覆盖阶数计算；连续分裂覆盖从高阶降到低阶的链表迁移；伙伴与非伙伴释放对比可以防止错误合并；失败申请和最终页数恢复用于检查失败原子性及内存泄漏。因此测试不仅检查“能否返回指针”，还检查算法最关键的不变量。

## 提示词 5：SLUB 模块实现与内核接入

**负责人：2412763-王泽鼎**

```text
[PROMPT]
任务：在 kern/mm/slub.c 中实现教学用的简化两层 SLUB，使内核能够在底层物理页
分配器之上申请和释放小对象；大对象使用连续页回退路径。实现必须复用现有页分配
接口，不能绕过 pmm_manager 自行管理未登记的物理内存。

操作要求：
1. 提供 slub_init、slub_alloc、slub_free 和 slub_check 所需接口。
2. 小对象使用 16、32、64、128、256、512、1024、2048 字节 size class。
3. 在 slab 页首保存必要元数据，在空闲对象内部保存 freelist next 指针。
4. size=0 明确返回 NULL；大于 2048 字节的请求走连续多页路径并保存实际页数。
5. 指针换算必须符合内核直接映射关系；处理对齐和元数据占用，不能把元数据当对象返回。
6. 一个 slab 页完全空闲后应从 cache 链表摘除并归还底层页分配器。

[RELY]
- alloc_pages、free_pages、nr_free_pages。
- page2pa、pa2page、va_pa_offset 和 4 KiB 页大小。
- list.h 链表操作及内核现有断言/输出接口。

[GUARANTEE]
- 小对象选择能够容纳 size 的最小 size class。
- 没有可用对象时申请新 slab 页并建立空闲对象链表。
- 分配后对象不重叠，并满足基本对齐和可写要求。
- 释放对象回到正确 slab；整页全空时归还底层。
- 大对象记录连续页数，释放时完整归还对应页。

[SPECIFICATION]
Case 1：size=0，返回 NULL 且底层空闲页数不变。
Case 2：size<=2048，选择最小可容纳 size class；无可用 slab 时创建新页。
Case 3：size>2048，按“管理头+载荷”总长度向上取整到页数，申请连续页。
Case 4：释放小对象，恢复 freelist 与 inuse 计数；inuse=0 时归还整页。
Case 5：释放大对象，根据头部保存的页数一次性归还，不遗漏也不重复释放。
Post-Condition：任意仍存活对象互不重叠，元数据和页计数保持一致。
```

## 提示词 6：SLUB 测试设计与生成

**负责人：2412763-王泽鼎**

```text
[PROMPT]
任务：为简化 SLUB 设计并生成独立自动测试，验证 size class 选择、同页多对象、跨 slab、
大对象回退、数据可写性、释放回收和错误边界。不得修改实现来迎合测试，也不得只用
一次分配成功作为结论。测试接入 make challenge-test，并给出可定位失败阶段的输出。

操作要求：
1. 测试前保存 nr_free_pages，结束前释放所有成功分配的对象。
2. 对返回对象写入并读回不同字节模式，验证对象可写且互不覆盖。
3. 测试尺寸应覆盖边界值，而不是只测 size class 的正中间。
4. 每次失败后安全清理已经成功分配的对象，避免测试自身造成泄漏。

[RELY]
- slub_init、slub_alloc、slub_free。
- nr_free_pages 和 QEMU 控制台输出。
- Makefile 的 Challenge 构建与运行入口。

[GUARANTEE]
测试至少覆盖：
- size=0；
- 1、16、17、32、33、2048、2049 等分级边界；
- 同一 size class 的多个对象；
- 对象数量超过单页容量，触发新的 slab 页；
- 多个 size class 同时存活；
- 大对象连续多页分配和释放；
- 交错释放及 slab 全空后整页回收；
- 全部释放后底层空闲页数恢复。

[SPECIFICATION]
Case 1（小对象）：对象属于正确 size class，地址互不重叠，写入内容互不干扰。
Case 2（跨 slab）：单页耗尽后能够创建新 slab，旧对象内容保持不变。
Case 3（大对象）：2049 字节及更大请求走多页路径，写入边界不破坏管理头。
Case 4（释放）：部分释放后其余对象仍有效；全部释放后相关 slab 页被归还。
Post-Condition：控制台输出 slub check succeeded，测试前后 nr_free_pages 相等。
```
