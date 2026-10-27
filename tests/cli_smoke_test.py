"""End-to-end CLI smoke test (Python standard library only)."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
EXECUTABLE = ROOT / ("csopesy.exe" if __import__("os").name == "nt" else "csopesy")
if not EXECUTABLE.is_file():
    raise SystemExit("Build the C++ executable before running this test.")

with tempfile.TemporaryDirectory() as tmp:
    folder = Path(tmp)
    shutil.copy(ROOT / "config-demo.txt", folder / "config.txt")
    child = subprocess.Popen([str(EXECUTABLE)], cwd=folder, stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             text=True, encoding="utf-8", errors="replace")
    assert child.stdin is not None
    def command(text: str, sleep: float = 0):
        child.stdin.write(text + "\n")
        child.stdin.flush()
        time.sleep(sleep)

    command("screen -ls")           # initialization gate
    command("initialize")
    command("screen -s demo")      # enters attached process screen
    command("process-smi")
    command("exit")                # detach
    command("scheduler-start", 0.35)
    command("scheduler-stop", 2.0)
    command("screen -ls")
    command("report-util")
    command("exit")
    output, errors = child.communicate(timeout=10)
    assert child.returncode == 0, errors
    assert "Please enter 'initialize' first" in output
    assert "Initialized 4 CPU" in output
    assert "Process name: demo" in output
    assert "Scheduler process generation started" in output
    assert "Scheduler process generation stopped" in output
    assert "Report generated at csopesy-log.txt" in output
    saved = (folder / "csopesy-log.txt").read_text(encoding="utf-8")
    assert "demo" in saved
    assert "Finished processes:" in saved
    assert "Running processes:" in saved
    assert "CPU utilization:" in saved

    # Human-readable, color-free transcript showing real terminal output.
    without_ansi = output.replace("\033[2J\033[H", "")
    (ROOT / "DEMO_TRANSCRIPT.txt").write_text(without_ansi + "\n--- REPORT FILE ---\n" + saved, encoding="utf-8")
    print("PASS: CLI startup, screen attach, scheduler generator, report file, clean exit")
