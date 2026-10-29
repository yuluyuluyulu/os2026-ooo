#!/usr/bin/env python3
"""Record the course grade, then independent runtime checks, sequentially."""
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parent.parent
os.chdir(root)
evidence = root.parent / "report" / "evidence"
evidence.mkdir(parents=True, exist_ok=True)
for target, filename in (("grade", "grade.log"), ("selftest", "selftest-run.log"),
                         ("challenge-test", "challenge-run.log"), ("shutdown-test", "shutdown-run.log")):
    print(f"=== make {target} ===", flush=True)
    result = subprocess.run(["make", "--no-print-directory", target], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (evidence / filename).write_text(result.stdout, encoding="utf-8")
    print(result.stdout, end="", flush=True)
    if result.returncode:
        raise SystemExit(result.returncode)
    if target == "grade" and "Total Score: 100/100" not in result.stdout:
        raise SystemExit("Course score is not 100/100")
print("Lab3 verification: PASS (course grade + selftest + challenge + shutdown)")
