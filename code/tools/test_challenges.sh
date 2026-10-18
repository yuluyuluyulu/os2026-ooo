#!/bin/sh
set -eu

log=.challenge.out
make --quiet --no-print-directory clean
make --quiet --no-print-directory DEFS="-DUSE_BUDDY_PMM -DENABLE_CHALLENGE_TESTS"

qemu-system-riscv64 \
    -machine virt -nographic -bios default -kernel bin/ucore.img >"$log" 2>&1 &
qemu_pid=$!

stop_qemu() {
    if kill -0 "$qemu_pid" 2>/dev/null; then
        kill "$qemu_pid" 2>/dev/null || true
    fi
    wait "$qemu_pid" 2>/dev/null || true
}
trap stop_qemu EXIT INT TERM

# A cold QEMU/OpenSBI start can exceed 12 seconds on a busy WSL host.  Poll the
# actual completion marker for at most 30 seconds and stop as soon as it appears.
elapsed=0
while [ "$elapsed" -lt 30 ]; do
    if grep -F "slub allocator check succeeded!" "$log" >/dev/null 2>&1; then
        break
    fi
    if ! kill -0 "$qemu_pid" 2>/dev/null; then
        break
    fi
    sleep 1
    elapsed=$((elapsed + 1))
done

stop_qemu
trap - EXIT INT TERM

require_output() {
    if ! grep -F "$1" "$log" >/dev/null 2>&1; then
        echo "Challenge test failed: missing output: $1" >&2
        echo "----- QEMU output -----" >&2
        cat "$log" >&2
        exit 1
    fi
}

require_output "memory management: buddy_system_pmm_manager"
require_output "buddy_system check succeeded!"
require_output "slub allocator check succeeded!"
require_output "check_alloc_page() succeeded!"

echo "-check buddy system: OK"
echo "-check slub allocator: OK"
echo "Challenge tests: PASS"
