# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| **实验名称** | Lab 2：物理内存管理与 Best-Fit 连续页分配 |
| **小组成员** | 2412930-林永圣、2414063-尹天宇、2412763-王泽鼎 |
| **完成日期** | 2026-10-18 |

### 小组分工

| 成员 | 负责的练习/模块 |
|------|----------------|
| 2412930-林永圣 | 物理内存探测结果接入、`page_init`、页描述符数组与空闲区初始化 |
| 2414063-尹天宇 | Best-Fit 分配器的初始化、分配、释放、相邻块合并和管理器注册 |
| 2412763-王泽鼎 | Sv39 启动映射兼容性、Makefile/QEMU/GDB 调试、自动测试与报告证据整理 |

---

## 一、实验目的

1. 理解物理地址、虚拟地址、页、页帧和页描述符之间的关系。
2. 理解 RISC-V Sv39 地址翻译、`satp` 和 `sfence.vma` 的作用。
3. 根据设备树给出的物理内存范围建立 `struct Page` 描述符数组。
4. 正确排除 OpenSBI、内核镜像和页描述符数组占用的空间，并把剩余整页交给物理内存管理器。
5. 实现 Best-Fit 连续物理页分配算法，并在释放时按地址排序、合并相邻空闲块。
6. 使用 QEMU、课程随包测试和 GDB 验证从物理入口切换到高地址内核以及内存分配器行为。

---

## 二、实验环境

| 组件 | 实际环境/用途 |
|------|---------------|
| 宿主环境 | Windows + WSL2 Ubuntu |
| 目标架构 | RISC-V 64 位（RV64） |
| 交叉工具链 | `riscv64-unknown-elf-gcc/ld/objcopy` |
| 模拟器与固件 | `qemu-system-riscv64` + OpenSBI v1.0 |
| 调试器 | `riscv64-unknown-elf-gdb`，目标架构 `riscv:rv64` |
| 构建工具 | GNU Make |
| 实验代码 | 课程提供的 `lab2.zip` |
| AI 辅助工具 | Codex 桌面端；结果由源码、测试与调试共同复核 |

课程指导书参考：<https://nankai.gitbook.io/ucore-os-on-risc-v64/lab2>

---

## 三、实验整体逻辑

### 3.1 从启动到物理页分配

```text
QEMU 加载 OpenSBI 与 ucore.img
    ↓
OpenSBI 在 M-mode 初始化平台
    ↓ 跳转到物理地址 0x80200000，进入 S-mode
kern_entry 保存 hartid/DTB 地址
    ↓
设置 Sv39 根页表，刷新 TLB，跳到高虚拟地址
    ↓
kern_init 解析 DTB，得到物理内存范围
    ↓
pmm_init 选择 Best-Fit 管理器
    ↓
page_init 建立 Page 数组并登记可用空闲页
    ↓
Best-Fit 自检、课程注入测试
```

实验把“地址翻译机制”和“物理页分配策略”分开：启动页表负责让 CPU 能用虚拟地址访问内核；`pmm_manager` 负责管理哪些物理页空闲、如何分配和回收。

### 3.2 核心数据结构

`struct Page` 并不是一页内存本身，而是某个物理页帧的管理记录：

```c
struct Page {
    int ref;
    uint64_t flags;
    unsigned int property;
    list_entry_t page_link;
};
```

- `ref`：页引用计数。
- `PG_reserved`：该页被内核等占用，不能分配。
- `PG_property`：该描述符是一个空闲连续块的首元素。
- `property`：仅在空闲块首元素中有效，表示连续空闲页数。
- `page_link`：把各空闲块首元素连接成双向链表。

`free_area_t` 同时维护空闲块链表 `free_list` 和空闲页总数 `nr_free`。链表按物理地址递增排列，方便释放时判断前后块是否相邻。

---

## 四、功能一：物理内存初始化

**负责人：2412930-林永圣**

### 4.1 功能目标

`page_init` 需要把 DTB 报告的物理内存转换成内核可管理的页集合，同时保证固件、内核和管理元数据不会被误分配。

本机 DTB 实测结果为：

- 起始物理地址：`0x80000000`
- 大小：`0x08000000`，即 128 MiB
- 结束边界：`0x88000000`（半开区间，不包含该地址）
- 页大小：4 KiB
- 总页号上界：`PPN(0x88000000)`

### 4.2 实现过程

1. 将 `va_pa_offset` 设置为 `PHYSICAL_MEMORY_OFFSET`，使 `PADDR(kva)` 能把高虚拟地址换算为物理地址。
2. 从 `get_memory_base()` 和 `get_memory_size()` 取得 DTB 的探测结果，计算物理内存半开区间。
3. 设置 `npage = PPN(mem_end)`；`pages[0]` 对应 `nbase = PPN(0x80000000)`，因此转换关系为：

   ```text
   page2ppn(page) = page - pages + nbase
   ```

4. 将 `pages` 放在链接器符号 `end` 之后并按 4 KiB 对齐，为每个 DRAM 页建立一个 `struct Page`。
5. 先把全部描述符标记为保留。这样内核镜像、页描述符数组及其对齐空隙默认不会进入空闲链表。
6. 计算描述符数组结束后的第一个完整物理页，只将 `[free_mem, mem_end)` 交给 `init_memmap`。

### 4.3 前置与后置状态

**前置条件：** `dtb_init` 已成功解析内存节点；链接器提供 `end`；高地址内核映射已经生效。

**成功后的状态：**

- `va_pa_offset` 正确反映虚实地址差值；
- `npage` 与 DTB 内存末端一致；
- `pages` 非空、按页对齐且位于内核末尾之后；
- 内核尾页和描述符数组所在页仍为 reserved；
- 空闲链表非空，且 `page2pa/pa2page` 可以往返转换。

若 DTB 内存范围非法、可用空间不足或地址假设不成立，内核使用断言停止，而不是建立可能破坏内核的错误空闲表。

---

## 五、功能二：Best-Fit 连续物理页分配

**负责人：2414063-尹天宇**

### 5.1 算法含义

请求连续 `n` 页时，Best-Fit 遍历全部空闲块，在所有 `property >= n` 的候选块中选择尺寸最小者。若存在恰好为 `n` 页的块，可以立即停止搜索。该策略尽量避免把很大的空闲块用于很小的请求。

例如空闲块大小依次为 8、3、5 页，请求 4 页：First-Fit 会选择 8 页块，Best-Fit 会选择 5 页块，并留下 1 页空闲块。

### 5.2 初始化与注册

- `best_fit_init` 初始化空闲链表并将 `nr_free` 清零。
- `best_fit_init_memmap` 清除可用页的 reserved 标志，把整个初始可用区登记为一个空闲块。
- `init_pmm_manager` 改为选择 `best_fit_pmm_manager`，使上层继续通过统一的 `alloc_pages/free_pages` 机制调用新策略。

### 5.3 分配流程

1. 断言 `n > 0`。
2. 若 `n > nr_free`，立即返回 `NULL`，且不得改变链表和计数。
3. 遍历链表，记录当前满足要求且尺寸最小的块。
4. 从链表摘除选中的原块。
5. 若块大于请求，将剩余部分设置为新的空闲块首元素，并放回原地址顺序位置。
6. 清除已分配块首元素的 `PG_property`，更新 `nr_free`，返回块起始页。

### 5.4 释放与合并

释放连续 `n` 页时，先检查各页既不是 reserved，也不是其他空闲块的首元素；随后清理引用和属性，把 `base` 标记成新的空闲块首元素。

新块按地址插入链表后依次尝试：

- 若前一块末端等于 `base`，合并到前一块；
- 若合并后块的末端等于后一块，再合并后一块。

这样一次释放最多完成左右两次合并，避免产生可合并却分散的外部碎片，并保持 `nr_free` 等于所有空闲块 `property` 之和。

---

## 六、Sv39 启动映射与兼容性修正

**负责人：2412763-王泽鼎**

### 6.1 页表与 `satp`

Sv39 使用三级页表，普通页为 4 KiB。根页表一页包含 512 个 8 字节页表项；顶级叶子项可直接映射 1 GiB 区域。`satp` 的 MODE=8 表示 Sv39，低 44 位保存根页表物理页号。修改 `satp` 后使用 `sfence.vma` 刷新旧的 TLB 翻译结果。

内核链接在高虚拟地址 `0xffffffffc0200000`，物理入口为 `0x80200000`，两者相差 `0xffffffff40000000`。

### 6.2 发现的问题

原压缩包使用 `-device loader,file=...,addr=0x80200000`。本机 QEMU/OpenSBI 输出 `Domain0 Next Address = 0x0`，不会把控制权交给内核。改用 `-kernel bin/ucore.img` 后，下一跳变为 `0x80200000`。

随后发现原启动页表只映射高地址区域。CPU 在物理地址 `0x80200000` 执行 `csrw satp` 和 `sfence.vma` 后，下一条指令仍从低地址 PC 取指；若没有临时恒等映射，低地址会立刻变成未映射地址。

### 6.3 修正方法

根页表同时建立两个指向物理 `0x80000000` 的 1 GiB 叶子项：

- VPN[2] = 2：临时恒等映射 `0x80000000 → 0x80000000`，保证开启分页后的过渡指令可执行；
- VPN[2] = 511：高地址映射 `0xffffffffc0000000 → 0x80000000`，供内核正式运行。

随后汇编代码把 `sp` 和跳转目标设置为高虚拟地址，进入 `kern_init`。GDB 实测 `satp = 0x8000000000080205`：最高位域表示 Sv39，根页表物理页号为 `0x80205`，对应物理地址 `0x80205000`。

---

## 七、实验结果与验证

### 7.1 编译

在 `code/` 中执行：

```bash
make clean
make
```

所有 C/汇编文件均通过 `-Werror` 编译，成功生成 `bin/kernel` 与 `bin/ucore.img`。

![Lab2 编译成功](images/01-build-success.png)

### 7.2 QEMU 启动和内存初始化

执行 `make qemu` 后，实测关键输出：

```text
Domain0 Next Address      : 0x0000000080200000
Domain0 Next Mode         : S-mode
Physical Memory from DTB:
  Base: 0x0000000080000000
  Size: 0x0000000008000000 (128 MB)
memory management: best_fit_pmm_manager
free memory: 0x0000000080347000 - 0x0000000088000000 (31929 pages)
check_alloc_page() succeeded!
```

![QEMU 启动与物理内存初始化](images/02-qemu-memory-init.png)

末尾的 timeout 终止信息是测试脚本主动结束 QEMU；内核按设计进入永久循环，并非崩溃。

### 7.3 课程随包测试

执行 `make grade`。该目标复制代码到 `/tmp/lab2_grade`，注入压缩包自带的测试文件，再重新编译运行；未修改 `basic_check`、`best_fit_check` 或注入测试内容。

```text
-check page init: OK
-check best fit:  OK
Total Score: 20/20
```

![课程随包测试 20/20](images/03-grade-20-of-20.png)

该分数表示本地压缩包附带测试全部通过，不替代教师平台的最终评分。

### 7.4 GDB 调试

执行：

```bash
sh tools/run_gdb_evidence.sh
```

调试结果形成了完整的地址切换证据：

- 复位 PC：`0x1000`；
- OpenSBI 交接后的物理入口：`0x80200000`；
- 开启 Sv39 后的 `kern_init`：`0xffffffffc02000d6`；
- `sp`：`0xffffffffc0205000`；
- `bootstacktop`：`0xffffffffc0205000`；
- `satp`：`0x8000000000080205`。

![GDB 验证物理入口与高地址内核](images/04-gdb-sv39-transition.png)

---

## 八、运行与复现实验步骤

在 WSL 中进入仓库的 `code` 目录：

```bash
cd /mnt/d/cc-dp/os/code
```

### 8.1 编译

```bash
make clean
make
```

### 8.2 启动

```bash
make qemu
```

看到 `check_alloc_page() succeeded!` 即说明内核启动和分配器自检完成。内核不会自行退出；按 `Ctrl+A`，松开后按 `X` 退出 QEMU。

### 8.3 自动测试

```bash
make grade
```

预期最后出现：

```text
Total Score: 20/20
```

### 8.4 GDB 手动调试

终端 1：

```bash
make debug
```

终端 2：

```bash
make gdb
```

每条 GDB 命令输入后单独回车，例如：

```gdb
set pagination off
p/x $pc
x/5i $pc
b *0x80200000
hbreak kern_init
c
p/x $pc
p/x $sp
p/x &bootstacktop
p/x $satp
```

也可用 `sh tools/run_gdb_evidence.sh` 自动完成同一组检查并生成日志。

---

## 九、AI 协作、验证与迭代说明

本实验按照课程视频建议，以职责闭合的功能模块为提示词粒度，并使用 `PROMPT / RELY / GUARANTEE / SPECIFICATION` 四段式描述需求。AI 负责代码实现建议、差异检查和故障定位；小组成员通过以下方式复核：

1. 阅读 `Page`、`free_area_t`、地址转换宏和课程测试，确认接口语义。
2. 用断言和课程注入测试检查成功、失败、拆分、最优选择和相邻合并。
3. 观察 OpenSBI 的下一跳、DTB 内存范围与内核实际输出。
4. 用 GDB 直接检查复位 PC、物理入口、高虚拟地址、栈顶和 `satp`。
5. 对“只显示 OpenSBI”和“开启分页后无输出”分别迭代提示词与实现，而不是绕过测试或伪造结果。

完整提示词记录见 `report/prompt.md`。

---

## 十、总结

本实验完成了两项核心功能：依据 DTB 和内核内存布局建立物理页管理元数据；实现并启用 Best-Fit 连续物理页分配器。通过地址有序链表、块拆分和双向合并，分配器在保持计数一致的同时满足最小合适块语义。

在工程验证中还解决了本机 QEMU/OpenSBI 镜像交接方式和 Sv39 开启瞬间的低地址取指问题。最终构建成功，QEMU 输出正常，内置检查通过，课程随包测试为 20/20，GDB 验证了 `0x1000 → 0x80200000 → 0xffffffffc02000d6` 的启动与地址空间切换链路。
