#!/usr/bin/env python3
"""Build and exercise real guest traps; never inject or change course tests."""
import os
from pathlib import Path
import subprocess
import sys

mode = sys.argv[1] if len(sys.argv) == 2 else "selftest"
if mode not in ("selftest", "challenge", "shutdown"):
    raise SystemExit("usage: test_runtime.py selftest|challenge|shutdown")
root = Path(__file__).resolve().parent.parent
os.chdir(root)
evidence = root / ".test-output"
evidence.mkdir(parents=True, exist_ok=True)
definitions = {"selftest": "-DLAB3_SELFTEST", "challenge": "-DLAB3_SELFTEST -DLAB3_CHALLENGE", "shutdown": ""}
build = []
for command in (["make", "--no-print-directory", "clean"],
                ["make", "--no-print-directory", "DEFS=" + definitions[mode]]):
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    build.append(result.stdout)
    if result.returncode:
        (evidence / (mode + "-build.log")).write_text("".join(build), encoding="utf-8")
        print(result.stdout)
        raise SystemExit(result.returncode)
(evidence / (mode + "-build.log")).write_text("".join(build), encoding="utf-8")
qemu = os.environ.get("QEMU", "qemu-system-riscv64")
command = [qemu, "-machine", "virt", "-nographic", "-bios", "default", "-kernel", "bin/ucore.img", "-monitor", "none", "-no-reboot"]
try:
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=22)
except subprocess.TimeoutExpired as exc:
    output = (exc.stdout or b"").decode(errors="replace")
    (evidence / (mode + ".log")).write_text(output, encoding="utf-8")
    print(output)
    raise SystemExit("FAIL: QEMU did not shut down within 22 seconds")
output = result.stdout.decode(errors="replace")
(evidence / (mode + ".log")).write_text(output, encoding="utf-8")
print(output, end="")
required = ["check_alloc_page() succeeded!", "++ setup timer interrupts", "Timer shutdown: 1000 ticks, 10 reports"]
if mode != "shutdown":
    required += ["[Selftest] all 32 GPR slots, 16/32-bit breakpoints: PASS",
                 "[Selftest] 16/32-bit illegal instruction EPC: PASS",
                 "[Selftest] allocator SIE preservation and page conservation: PASS"]
if mode == "challenge":
    required += ["[Challenge] S -> U:", "[Challenge] U timer:",
                 "[Challenge] U privileged CSR: denied", "[Challenge] U read kernel: denied",
                 "[Challenge] U -> S:", "[Challenge] mode switch: PASS"]
missing = [marker for marker in required if marker not in output]
reports = sum(line.strip() == "100 ticks" for line in output.splitlines())
if result.returncode or missing or reports != 10 or "panic" in output.lower() or "FAIL" in output:
    raise SystemExit(f"FAIL: return={result.returncode}, reports={reports}, missing={missing}")
print(f"{mode.capitalize()} tests: PASS (10 timer reports, clean SBI shutdown)")
