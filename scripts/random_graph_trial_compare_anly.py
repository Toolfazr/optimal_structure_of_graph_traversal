import csv
import re
import sys
from pathlib import Path
from collections import defaultdict
import matplotlib.pyplot as plt

# ========== 处理超长字段 ==========
max_int = sys.maxsize
while True:
    try:
        csv.field_size_limit(max_int)
        break
    except OverflowError:
        max_int //= 10

# ========== 路径 ==========
ROOT = Path(__file__).parent.parent / "TrialRes" / "RandomGraphTrial"
SUMMARY_FILE = ROOT / "random_graph_trial_summary_by_case.csv"

# general_distribution_{AdjList/AdjMatrix}_{DFS/BFS}_n_p_1.csv
FILE_RE = re.compile(
    r"general_distribution_(AdjList|AdjMatrix)_(DFS|BFS)_(\d+)_([0-9.]+)_(\d+)\.csv"
)

SITUATIONS = [
    ("AdjList", "DFS"),
    ("AdjList", "BFS"),
    ("AdjMatrix", "DFS"),
    ("AdjMatrix", "BFS"),
]

N_VALUES = [8, 9, 10, 11, 12]
P_VALUES = [0.3, 0.4, 0.5, 0.6, 0.7]


# =========================================================
# 工具函数
# =========================================================

def parse_p_str_to_float(p_str: str) -> float:
    return round(float(p_str), 1)


def format_p(p: float) -> str:
    return f"{p:.1f}"


def normalize_storage(s: str):
    s = s.strip()
    mapping = {
        "AdjList": "AdjList",
        "AdjMatrix": "AdjMatrix",
        "List": "AdjList",
        "Matrix": "AdjMatrix",
    }
    return mapping.get(s, s)


def normalize_traversal(s: str):
    s = s.strip()
    mapping = {
        "DFS": "DFS",
        "BFS": "BFS",
    }
    return mapping.get(s, s)


def find_column(fieldnames, candidates):
    for name in candidates:
        if name in fieldnames:
            return name
    return None


def ensure_dir(path: Path):
    path.mkdir(parents=True, exist_ok=True)


def load_csv_rows(path: Path):
    with path.open("r", encoding="utf-8", newline="") as f:
        return list(csv.DictReader(f))


def to_float(x):
    return float(x)


def to_int(x):
    return int(x)


# =========================================================
# Part 1: 统计 ReGraph 返回的 general_distribution_*.csv
# =========================================================

def load_distribution(path: Path):
    """
    只读取 key 和 count 两列。
    """
    dist = {}

    with path.open("r", encoding="utf-8") as f:
        header = next(f).strip().split(",")
        try:
            key_idx = header.index("key")
            count_idx = header.index("count")
        except ValueError:
            raise ValueError(f"{path.name} 的表头中未找到 key 或 count 列")

        need_idx = max(key_idx, count_idx)

        for line in f:
            line = line.rstrip("\n")
            if not line.strip():
                continue

            parts = line.split(",", maxsplit=need_idx + 1)
            key = int(parts[key_idx].strip())
            count = int(parts[count_idx].strip())
            dist[key] = count

    return dist


def analyze_general_distribution_group(files):
    """
    对一组 general_distribution_*.csv 做统计：
    - total: 所有访问秩总数（count 总和）
    - best: 每个 csv 中 key 最小时对应的 count，跨文件累加
    - second: 每个 csv 中 key 次小时对应的 count，跨文件累加
    """
    total = 0
    best = 0
    second = 0

    for path in files:
        dist = load_distribution(path)
        if not dist:
            continue

        keys = sorted(dist.keys())
        best_key = keys[0]
        second_key = keys[1] if len(keys) > 1 else None

        total += sum(dist.values())
        best += dist[best_key]

        if second_key is not None:
            second += dist[second_key]

    return {
        "total": total,
        "best": best,
        "second": second,
        "best_ratio": best / total if total else 0.0,
        "second_ratio": second / total if total else 0.0,
    }


def collect_general_distribution_files():
    files_by_situation = defaultdict(list)
    files_by_n = defaultdict(list)
    files_by_p = defaultdict(list)

    for path in ROOT.glob("general_distribution_*.csv"):
        m = FILE_RE.match(path.name)
        if not m:
            continue

        storage, traversal, n_str, p_str, trial_no = m.groups()
        n = int(n_str)
        p = parse_p_str_to_float(p_str)

        files_by_situation[(storage, traversal)].append(path)
        files_by_n[n].append(path)
        files_by_p[p].append(path)

    return files_by_situation, files_by_n, files_by_p


def summarize_general_distribution():
    files_by_situation, files_by_n, files_by_p = collect_general_distribution_files()

    # 1) 四种情形整体统计
    situation_rows = []
    for storage, traversal in SITUATIONS:
        files = files_by_situation.get((storage, traversal), [])
        result = analyze_general_distribution_group(files)

        situation_rows.append({
            "storage": storage,
            "traversal": traversal,
            "file_count": len(files),
            "total": result["total"],
            "best": result["best"],
            "second": result["second"],
            "best_ratio": result["best_ratio"],
            "second_ratio": result["second_ratio"],
        })

    # 2) 只按 n 分图统计，不分情形
    n_rows = []
    for n in N_VALUES:
        files = files_by_n.get(n, [])
        result = analyze_general_distribution_group(files)

        n_rows.append({
            "n": n,
            "file_count": len(files),
            "total": result["total"],
            "best": result["best"],
            "second": result["second"],
            "best_ratio": result["best_ratio"],
            "second_ratio": result["second_ratio"],
        })

    # 3) 只按 p 分图统计，不分情形
    p_rows = []
    for p in P_VALUES:
        files = files_by_p.get(p, [])
        result = analyze_general_distribution_group(files)

        p_rows.append({
            "p": format_p(p),
            "file_count": len(files),
            "total": result["total"],
            "best": result["best"],
            "second": result["second"],
            "best_ratio": result["best_ratio"],
            "second_ratio": result["second_ratio"],
        })

    out1 = ROOT / "general_distribution_summary_by_situation.csv"
    with out1.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "storage", "traversal", "file_count",
                "total", "best", "second",
                "best_ratio", "second_ratio"
            ]
        )
        writer.writeheader()
        writer.writerows(situation_rows)

    out2 = ROOT / "general_distribution_summary_by_n.csv"
    with out2.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "n", "file_count",
                "total", "best", "second",
                "best_ratio", "second_ratio"
            ]
        )
        writer.writeheader()
        writer.writerows(n_rows)

    out3 = ROOT / "general_distribution_summary_by_p.csv"
    with out3.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "p", "file_count",
                "total", "best", "second",
                "best_ratio", "second_ratio"
            ]
        )
        writer.writeheader()
        writer.writerows(p_rows)

    return situation_rows, n_rows, p_rows, out1, out2, out3


# =========================================================
# Part 2: 统计方法框架结果 random_graph_trial_summary_by_case.csv
# =========================================================

def summarize_method_framework():
    if not SUMMARY_FILE.exists():
        raise FileNotFoundError(f"未找到文件: {SUMMARY_FILE}")

    grouped_by_situation = defaultdict(lambda: {
        "case_count": 0,
        "total": 0,
        "best": 0,
        "second": 0,
    })

    grouped_by_n = defaultdict(lambda: {
        "case_count": 0,
        "total": 0,
        "best": 0,
        "second": 0,
    })

    grouped_by_p = defaultdict(lambda: {
        "case_count": 0,
        "total": 0,
        "best": 0,
        "second": 0,
    })

    with SUMMARY_FILE.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        fieldnames = reader.fieldnames or []

        storage_col = find_column(fieldnames, ["storage"])
        traversal_col = find_column(fieldnames, ["traversal"])
        n_col = find_column(fieldnames, ["n"])
        p_col = find_column(fieldnames, ["p"])
        opt_rank_count_col = find_column(fieldnames, ["opt_rank_count"])
        hit_best_col = find_column(fieldnames, ["hit_best_space"])
        hit_second_col = find_column(fieldnames, ["_hit_second_space", "hit_second_space"])

        required = {
            "storage": storage_col,
            "traversal": traversal_col,
            "n": n_col,
            "p": p_col,
            "opt_rank_count": opt_rank_count_col,
            "hit_best_space": hit_best_col,
            "_hit_second_space/hit_second_space": hit_second_col,
        }
        missing = [k for k, v in required.items() if v is None]
        if missing:
            raise ValueError(
                f"{SUMMARY_FILE.name} 缺少必要列: {missing}；现有列为: {fieldnames}"
            )

        for row in reader:
            storage = normalize_storage(row[storage_col])
            traversal = normalize_traversal(row[traversal_col])
            n = int(row[n_col])
            p = parse_p_str_to_float(row[p_col])

            opt_rank_count = int(row[opt_rank_count_col])
            hit_best_space = int(row[hit_best_col])
            hit_second_space = int(row[hit_second_col])

            key1 = (storage, traversal)

            grouped_by_situation[key1]["case_count"] += 1
            grouped_by_situation[key1]["total"] += opt_rank_count
            grouped_by_situation[key1]["best"] += hit_best_space
            grouped_by_situation[key1]["second"] += hit_second_space

            grouped_by_n[n]["case_count"] += 1
            grouped_by_n[n]["total"] += opt_rank_count
            grouped_by_n[n]["best"] += hit_best_space
            grouped_by_n[n]["second"] += hit_second_space

            grouped_by_p[p]["case_count"] += 1
            grouped_by_p[p]["total"] += opt_rank_count
            grouped_by_p[p]["best"] += hit_best_space
            grouped_by_p[p]["second"] += hit_second_space

    # 1) 四种情形整体统计
    situation_rows = []
    for storage, traversal in SITUATIONS:
        data = grouped_by_situation[(storage, traversal)]
        total = data["total"]
        best = data["best"]
        second = data["second"]

        situation_rows.append({
            "storage": storage,
            "traversal": traversal,
            "case_count": data["case_count"],
            "total": total,
            "best": best,
            "second": second,
            "best_ratio": best / total if total else 0.0,
            "second_ratio": second / total if total else 0.0,
        })

    # 2) 只按 n 分图统计，不分情形
    n_rows = []
    for n in N_VALUES:
        data = grouped_by_n[n]
        total = data["total"]
        best = data["best"]
        second = data["second"]

        n_rows.append({
            "n": n,
            "case_count": data["case_count"],
            "total": total,
            "best": best,
            "second": second,
            "best_ratio": best / total if total else 0.0,
            "second_ratio": second / total if total else 0.0,
        })

    # 3) 只按 p 分图统计，不分情形
    p_rows = []
    for p in P_VALUES:
        data = grouped_by_p[p]
        total = data["total"]
        best = data["best"]
        second = data["second"]

        p_rows.append({
            "p": format_p(p),
            "case_count": data["case_count"],
            "total": total,
            "best": best,
            "second": second,
            "best_ratio": best / total if total else 0.0,
            "second_ratio": second / total if total else 0.0,
        })

    out1 = ROOT / "method_framework_summary_by_situation.csv"
    with out1.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "storage", "traversal", "case_count",
                "total", "best", "second",
                "best_ratio", "second_ratio"
            ]
        )
        writer.writeheader()
        writer.writerows(situation_rows)

    out2 = ROOT / "method_framework_summary_by_n.csv"
    with out2.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "n", "case_count",
                "total", "best", "second",
                "best_ratio", "second_ratio"
            ]
        )
        writer.writeheader()
        writer.writerows(n_rows)

    out3 = ROOT / "method_framework_summary_by_p.csv"
    with out3.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "p", "case_count",
                "total", "best", "second",
                "best_ratio", "second_ratio"
            ]
        )
        writer.writeheader()
        writer.writerows(p_rows)

    return situation_rows, n_rows, p_rows, out1, out2, out3


# =========================================================
# 可视化
# =========================================================

def plot_combined_bar(
    labels,
    baseline_best,
    baseline_second,
    method_best,
    method_second,
    title,
    save_path: Path
):
    x = list(range(len(labels)))
    width = 0.18

    plt.figure(figsize=(12, 6))

    plt.bar(
        [i - 1.5 * width for i in x],
        baseline_best,
        width=width,
        label="Random Baseline (best)"
    )
    plt.bar(
        [i - 0.5 * width for i in x],
        baseline_second,
        width=width,
        label="Random Baseline (second)"
    )
    plt.bar(
        [i + 0.5 * width for i in x],
        method_best,
        width=width,
        label="Proposed Method (best)"
    )
    plt.bar(
        [i + 1.5 * width for i in x],
        method_second,
        width=width,
        label="Proposed Method (second)"
    )

    plt.xticks(x, labels)
    plt.ylabel("ratio")
    plt.title(title)
    plt.legend()
    plt.tight_layout()
    plt.savefig(save_path, dpi=300)
    plt.close()


def visualize_by_situation(root: Path):
    baseline_rows = load_csv_rows(root / "general_distribution_summary_by_situation.csv")
    method_rows = load_csv_rows(root / "method_framework_summary_by_situation.csv")

    baseline_map = {(r["storage"], r["traversal"]): r for r in baseline_rows}
    method_map = {(r["storage"], r["traversal"]): r for r in method_rows}

    labels = []
    b_best, b_second = [], []
    m_best, m_second = [], []

    for storage, traversal in SITUATIONS:
        key = (storage, traversal)
        labels.append(f"{storage}\n{traversal}")

        b_best.append(float(baseline_map[key]["best_ratio"]))
        b_second.append(float(baseline_map[key]["second_ratio"]))

        m_best.append(float(method_map[key]["best_ratio"]))
        m_second.append(float(method_map[key]["second_ratio"]))

    fig_dir = root / "figures"
    ensure_dir(fig_dir)

    plot_combined_bar(
        labels,
        b_best, b_second,
        m_best, m_second,
        "Comparison by Situation",
        fig_dir / "compare_by_situation.png"
    )


def visualize_by_n(root: Path):
    baseline_rows = load_csv_rows(root / "general_distribution_summary_by_n.csv")
    method_rows = load_csv_rows(root / "method_framework_summary_by_n.csv")

    baseline_map = {int(r["n"]): r for r in baseline_rows}
    method_map = {int(r["n"]): r for r in method_rows}

    labels = []
    b_best, b_second = [], []
    m_best, m_second = [], []

    for n in N_VALUES:
        labels.append(str(n))

        b_best.append(float(baseline_map[n]["best_ratio"]))
        b_second.append(float(baseline_map[n]["second_ratio"]))

        m_best.append(float(method_map[n]["best_ratio"]))
        m_second.append(float(method_map[n]["second_ratio"]))

    fig_dir = root / "figures"
    ensure_dir(fig_dir)

    plot_combined_bar(
        labels,
        b_best, b_second,
        m_best, m_second,
        "Comparison by n",
        fig_dir / "compare_by_n.png"
    )


def visualize_by_p(root: Path):
    baseline_rows = load_csv_rows(root / "general_distribution_summary_by_p.csv")
    method_rows = load_csv_rows(root / "method_framework_summary_by_p.csv")

    baseline_map = {r["p"]: r for r in baseline_rows}
    method_map = {r["p"]: r for r in method_rows}

    labels = []
    b_best, b_second = [], []
    m_best, m_second = [], []

    for p in P_VALUES:
        p_str = format_p(p)
        labels.append(p_str)

        b_best.append(float(baseline_map[p_str]["best_ratio"]))
        b_second.append(float(baseline_map[p_str]["second_ratio"]))

        m_best.append(float(method_map[p_str]["best_ratio"]))
        m_second.append(float(method_map[p_str]["second_ratio"]))

    fig_dir = root / "figures"
    ensure_dir(fig_dir)

    plot_combined_bar(
        labels,
        b_best, b_second,
        m_best, m_second,
        "Comparison by p",
        fig_dir / "compare_by_p.png"
    )


def visualize_all(root: Path):
    visualize_by_situation(root)
    visualize_by_n(root)
    visualize_by_p(root)


# =========================================================
# 打印
# =========================================================

def print_rows(title, rows, count_field):
    print("\n" + "=" * 80)
    print(title)
    print("=" * 80)

    for row in rows:
        if "storage" in row and "traversal" in row:
            head = f"{row['storage']} + {row['traversal']}"
        elif "n" in row:
            head = f"n={row['n']}"
        else:
            head = f"p={row['p']}"

        print(head)
        print(f"{count_field} = {row[count_field]}")
        print(f"total = {row['total']}")
        print(f"best = {row['best']}")
        print(f"second = {row['second']}")
        print(f"best_ratio = {row['best_ratio']:.6f}")
        print(f"second_ratio = {row['second_ratio']:.6f}")
        print("-" * 80)


def main():
    print("ROOT =", ROOT.resolve())
    print("ROOT exists =", ROOT.exists())

    g_situation_rows, g_n_rows, g_p_rows, g_out1, g_out2, g_out3 = summarize_general_distribution()
    m_situation_rows, m_n_rows, m_p_rows, m_out1, m_out2, m_out3 = summarize_method_framework()

    print_rows("ReGraph general_distribution 按情形统计", g_situation_rows, "file_count")
    print_rows("ReGraph general_distribution 按 n 统计", g_n_rows, "file_count")
    print_rows("ReGraph general_distribution 按 p 统计", g_p_rows, "file_count")

    print_rows("方法框架按情形统计", m_situation_rows, "case_count")
    print_rows("方法框架按 n 统计", m_n_rows, "case_count")
    print_rows("方法框架按 p 统计", m_p_rows, "case_count")

    print(f"\n已写出: {g_out1}")
    print(f"已写出: {g_out2}")
    print(f"已写出: {g_out3}")
    print(f"已写出: {m_out1}")
    print(f"已写出: {m_out2}")
    print(f"已写出: {m_out3}")

    visualize_all(ROOT)
    print(f"已写出图像目录: {ROOT / 'figures'}")


if __name__ == "__main__":
    main()