import subprocess
from pathlib import Path

label = "bfs_list"

testcases = {
    12: [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9]
}

def run_testcases():
    project_root = Path(__file__).resolve().parent.parent  # scripts 的上一级
    exe = project_root / "TRIAL.exe"

    cmd = []
    for n, ps in testcases.items():
        for p in ps:
            cmd.append(f"\"{exe}\" {n} {p}")

    # 把多条命令拼成一条，由 shell 依次执行
    cmd = " & ".join(cmd)

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