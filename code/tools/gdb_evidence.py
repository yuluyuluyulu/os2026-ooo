#!/usr/bin/env python3
"""Observe a real compressed breakpoint before/after the C trap handler."""
import os
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
os.chdir(root)
evidence = root.parent / "report" / "evidence"
evidence.mkdir(parents=True, exist_ok=True)
for command in (["make", "--no-print-directory", "clean"],
                ["make", "--no-print-directory", "DEFS=-DLAB3_SELFTEST"]):
    subprocess.run(command, check=True)
gdb = shutil.which(os.environ.get("GCCPREFIX", "riscv64-unknown-elf-") + "gdb") or shutil.which("gdb-multiarch")
if not gdb:
    raise SystemExit("RISC-V GDB not found")
with socket.socket() as port_probe:
    port_probe.bind(("127.0.0.1", 0))
    port = port_probe.getsockname()[1]
commands = f"""set pagination off
set confirm off
set architecture riscv:rv64
file bin/kernel
target remote 127.0.0.1:{port}
break trap
continue
printf "=== C trap entry ===\\n"
print sizeof(struct trapframe)
print/x tf->cause
print/x tf->status
print/x tf->gpr.sp
print/x tf->epc
x/2hx tf->epc
set $saved_epc = tf->epc
set $frame = tf
disable 1
thbreak __trapret
continue
printf "=== after C handler ===\\n"
print/x $frame->epc
printf "EPC delta = %ld\\n", $frame->epc - $saved_epc
set $return_sp = $frame->gpr.sp
thbreak *$frame->epc
continue
printf "=== after sret ===\\n"
print/x $pc
print/x $sp
print/x $sscratch
printf "SP restored = %d\\n", $sp == $return_sp
x/i $pc
detach
quit
"""
with tempfile.TemporaryDirectory(prefix="lab3-gdb-") as temporary:
    script = Path(temporary) / "evidence.gdb"
    script.write_text(commands, encoding="utf-8")
    with (evidence / "gdb-qemu.log").open("wb") as serial:
        qemu = subprocess.Popen([os.environ.get("QEMU", "qemu-system-riscv64"),
                                 "-machine", "virt", "-nographic", "-bios", "default",
                                 "-kernel", "bin/ucore.img", "-monitor", "none",
                                 "-S", "-gdb", f"tcp:127.0.0.1:{port}"],
                                stdout=serial, stderr=subprocess.STDOUT)
        try:
            result = subprocess.run([gdb, "-q", "-batch", "-x", str(script)],
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, timeout=30)
        finally:
            qemu.terminate()
            try:
                qemu.wait(timeout=5)
            except subprocess.TimeoutExpired:
                qemu.kill()
                qemu.wait()
output = result.stdout
(evidence / "gdb.log").write_text(output, encoding="utf-8")
print(output, end="")
if result.returncode or "EPC delta = 2" not in output or "SP restored = 1" not in output:
    raise SystemExit("GDB evidence: FAIL")
print("GDB evidence: PASS")
