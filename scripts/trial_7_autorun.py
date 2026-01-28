import subprocess
from pathlib import Path
from collections import defaultdict

testcases = {
    12: [0.25, 0.25, 0.25, 0.25, 0.25, 0.35, 0.35, 0.35, 0.35, 0.35, 0.45, 0.45, 0.45, 0.45, 0.45],
}

project_root = Path(__file__).resolve().parent.parent

def run_testcases():
    exe = project_root / "TRIAL.exe"

    counter = defaultdict(int)
    cmds = []

    for n, ps in testcases.items():
        for p in ps:
            counter[(n, p)] += 1
            number = counter[(n, p)]
            cmds.append(f"\"{exe}\" {n} {p} {number}")

    cmd = " & ".join(cmds)
    print(">>", cmd)

    # 关键：不抓 stdout，让输出直接显示在当前控制台
    p = subprocess.Popen(
        cmd,
        shell=True,
        cwd=str(project_root),
    )
    p.wait()
    return p.returncode

def run_anlys():
    cmd = (
        "E:/Alas/ProgramLanguage/Python/Python3.8/python.exe "
        "./scripts/trial_7_anlys.py "
        "--dir ./TrialRes/Trial_7 "
        "--out ./TrialRes/Trial_7"
    )

    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        shell=True,
        cwd=str(project_root),
    )

    for line in process.stdout:
        print(line, end="")
    process.wait()
    return process.returncode

if __name__ == "__main__":
    run_testcases()
    run_anlys()