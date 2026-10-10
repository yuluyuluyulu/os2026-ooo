# riscv64-uCore LAB1

本目录来自课程提供的 `lab1.zip`，内容为 RISC-V64 最小可执行内核。

## 依赖

- `riscv64-unknown-elf-gcc`、`ld`、`objcopy`、`objdump`
- `qemu-system-riscv64`
- `make`
- `riscv64-unknown-elf-gdb` 或 `gdb-multiarch`（调试时需要）

## 使用

```bash
make          # 编译并生成 bin/kernel、bin/ucore.img
make qemu     # 通过 OpenSBI 启动内核；Ctrl+A 后按 X 退出
make debug    # 启动 QEMU GDB server，监听 1234 端口
make gdb      # 在另一终端连接 GDB
make grade    # 自动验证编译、链接地址和启动输出
make clean
```
