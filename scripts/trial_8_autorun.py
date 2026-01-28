import subprocess
import sys
from pathlib import Path
from collections import defaultdict

PROJECT_ROOT = Path(__file__).resolve().parent.parent
TRIAL_EXE = PROJECT_ROOT / "TRIAL.exe"
PYTHON = sys.executable

testcases = {
    8: [0.3, 0.4, 0.5, 0.6, 0.7],
    9: [0.3, 0.4, 0.5, 0.6, 0.7],
    10: [0.3, 0.4, 0.5, 0.6, 0.7],
    11: [0.3, 0.4, 0.5, 0.6, 0.7],
    12: [0.3, 0.4, 0.5, 0.6, 0.7],
}

def run_trials():
    counter = defaultdict(int)

    print("[autorun] executing trials...")
    for n, ps in testcases.items():
        for p in ps:
            counter[(n, p)] += 1
            number = counter[(n, p)]
            subprocess.run(
                [str(TRIAL_EXE), str(n), str(p), str(number)],
                cwd=str(PROJECT_ROOT),
                check=True,
            )

def run_analysis():
    print("[autorun] running analysis...")
    subprocess.run(
        [PYTHON, "scripts/trial_8_anlys.py"],
        cwd=str(PROJECT_ROOT),
        check=True,
    )

if __name__ == "__main__":
    # run_trials()
    run_analysis()