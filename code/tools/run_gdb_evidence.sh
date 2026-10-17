#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

make >/dev/null
mkdir -p .test-output

qemu-system-riscv64 \
    -machine virt -nographic -bios default -kernel bin/ucore.img \
    -s -S >.test-output/qemu-gdb.log 2>&1 &
qemu_pid=$!
trap 'kill "$qemu_pid" 2>/dev/null || true' EXIT INT TERM

sleep 2
gdb_cmd=$(make --quiet --no-print-directory print-gdb)

"$gdb_cmd" -batch bin/kernel \
    -ex 'set pagination off' \
    -ex 'set architecture riscv:rv64' \
    -ex 'target remote localhost:1234' \
    -ex 'printf "RESET_PC=%#lx\n", $pc' \
    -ex 'x/5i $pc' \
    -ex 'break *0x80200000' \
    -ex 'hbreak kern_init' \
    -ex 'continue' \
    -ex 'printf "PHYSICAL_ENTRY_PC=%#lx\n", $pc' \
    -ex 'x/16i $pc' \
    -ex 'continue' \
    -ex 'printf "KERN_INIT_PC=%#lx\n", $pc' \
    -ex 'printf "SP=%#lx\n", $sp' \
    -ex 'printf "BOOTSTACKTOP=%#lx\n", &bootstacktop' \
    -ex 'p/x $satp' \
    -ex 'detach' | tee .test-output/gdb-lab2.log
