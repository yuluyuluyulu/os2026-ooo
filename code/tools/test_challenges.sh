#!/bin/sh
set -eu

log=.challenge.out
make --quiet --no-print-directory clean
make --quiet --no-print-directory DEFS="-DUSE_BUDDY_PMM -DENABLE_CHALLENGE_TESTS"

set +e
timeout 12s qemu-system-riscv64 \
    -machine virt -nographic -bios default -kernel bin/ucore.img >"$log" 2>&1
status=$?
set -e
if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
    cat "$log"
    exit "$status"
fi

grep -F "memory management: buddy_system_pmm_manager" "$log" >/dev/null
grep -F "buddy_system check succeeded!" "$log" >/dev/null
grep -F "slub allocator check succeeded!" "$log" >/dev/null
grep -F "check_alloc_page() succeeded!" "$log" >/dev/null

echo "-check buddy system: OK"
echo "-check slub allocator: OK"
echo "Challenge tests: PASS"
