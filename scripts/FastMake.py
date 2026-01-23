import subprocess
import os
import sys

GCC = r"E:/Alas/ProgramLanguage/w64devkit/bin/gcc.exe"
GXX = r"E:/Alas/ProgramLanguage/w64devkit/bin/g++.exe"
MAKE = r"E:/Alas/ProgramLanguage/w64devkit/bin/mingw32-make.exe"

def run_cmd(cmd):
    env = os.environ.copy()
    env["PATH"] = r"E:\Alas\ProgramLanguage\w64devkit\bin;" + env["PATH"]
    print(">>", cmd if isinstance(cmd, str) else " ".join(cmd))
    p = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        shell=isinstance(cmd, str),
        env=env,
    )
    for line in p.stdout:
        print(line, end="")
    p.wait()
    return p.returncode

def main():
    # 用法：
    #   python scripts/FastMake.py                 -> SpaceTrial
    #   python scripts/FastMake.py SpaceTrial      -> SpaceTrial
    #   python scripts/FastMake.py Trial_1         -> Trial_1
    #   python scripts/FastMake.py Trial_2_1       -> Trial_2_1
    #   python scripts/FastMake.py Trial_2_2       -> Trial_2_2

    trial_name = sys.argv[1] if len(sys.argv) >= 2 else "SpaceTrial"
    trial_main = os.path.join(".", "src", "trials", f"{trial_name}.cpp")

    if not os.path.exists(trial_main):
        print(f"[ERROR] trial main not found: {trial_main}")
        print("Available trials in ./src/trials/:")
        try:
            for fn in sorted(os.listdir(os.path.join(".", "src", "trials"))):
                if fn.endswith(".cpp"):
                    print("  -", fn[:-4])
        except Exception:
            pass
        return 1

    cmake_cmd = [
    "cmake",
    "-G", "MinGW Makefiles",
    f"-DCMAKE_C_COMPILER={GCC}",
    f"-DCMAKE_CXX_COMPILER={GXX}",
    f"-DCMAKE_MAKE_PROGRAM={MAKE}",
    f"-DTRIAL_MAIN={trial_main}",
    "-S", ".",
    "-B", "build",
]


    build_cmd = ["cmake", "--build", "build"]

    ret = run_cmd(cmake_cmd)
    if ret == 0:
        ret = run_cmd(build_cmd)
    return ret

if __name__ == "__main__":
    raise SystemExit(main())
