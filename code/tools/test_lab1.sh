#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUTPUT_DIR=${TEST_OUTPUT_DIR:-"$ROOT_DIR/.test-output"}
BUILD_LOG="$OUTPUT_DIR/build.log"
LAYOUT_LOG="$OUTPUT_DIR/layout.log"
QEMU_LOG="$OUTPUT_DIR/qemu.log"

mkdir -p "$OUTPUT_DIR"
cd "$ROOT_DIR"

make clean >/dev/null 2>&1 || true
if ! make >"$BUILD_LOG" 2>&1; then
    cat "$BUILD_LOG"
    exit 1
fi

{
    echo "== section layout =="
    riscv64-unknown-elf-objdump -h bin/kernel
    echo "== key symbols =="
    riscv64-unknown-elf-objdump -t bin/kernel |
        grep -E 'kern_entry|kern_init|bootstack(top)?|edata|end$'
    echo "== entry disassembly =="
    riscv64-unknown-elf-objdump -d bin/kernel |
        sed -n '/<kern_entry>:/,/^$/p'
} >"$LAYOUT_LOG"

set +e
timeout 5s make qemu >"$QEMU_LOG" 2>&1
QEMU_STATUS=$?
set -e

if [ "$QEMU_STATUS" -ne 0 ] && [ "$QEMU_STATUS" -ne 124 ]; then
    cat "$QEMU_LOG"
    exit "$QEMU_STATUS"
fi

check_log() {
    LOG_FILE=$1
    EXPECTED=$2
    if ! grep -F "$EXPECTED" "$LOG_FILE" >/dev/null; then
        echo "FAIL: missing expected output: $EXPECTED" >&2
        cat "$LOG_FILE"
        exit 1
    fi
}

check_log "$LAYOUT_LOG" "0000000080200000"
check_log "$LAYOUT_LOG" "kern_entry"
check_log "$LAYOUT_LOG" "bootstacktop"
check_log "$QEMU_LOG" "Domain0 Next Address      : 0x0000000080200000"
check_log "$QEMU_LOG" "(THU.CST) os is loading ..."

echo "PASS: lab1 builds, links at 0x80200000, and boots through OpenSBI."
echo "Build log:  $BUILD_LOG"
echo "Layout log: $LAYOUT_LOG"
echo "QEMU log:   $QEMU_LOG"
