# 操作系统 Lab3

本次基于课程 `lab3.zip` 完成 Trap 上下文恢复、汇编入口、中断异常处理与时钟统计关机。

在 WSL Ubuntu 中运行：

```bash
cd /mnt/d/cc-dp/os/code
make clean
make
make qemu
```

默认内核每 100 ticks 输出一次，1000 ticks 后自动关机。

| 入口 | 用途 |
|------|------|
| `make grade` | 课程评分，预期 100/100 |
| `make selftest` | 16/32 位异常、寄存器与 SIE 自检 |
| `make gdb-evidence` | 真实 GDB Trap 返回证据 |
| `make shutdown-test` | 默认内核实际关机验证 |

报告见 [report/report.md](report/report.md)，提示词见 [report/prompt.md](report/prompt.md)，详细操作见 [启动与截图步骤](report/启动与截图步骤.md)。报告与提示词采用小组修改确认的版本，实际运行截图已补齐，GDB 使用小组手动操作截图。

旧 Lab2 代码与报告本地备份在被忽略的 `.local-backup/`，也保留在 Git 历史中。根目录原有的 `答辩参考.md` 和 `答辩参考/` 属于前次实验材料。
