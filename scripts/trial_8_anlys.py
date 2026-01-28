import csv
import re
from pathlib import Path

# 解决 Windows 下 field_size_limit(sys.maxsize) 会 Overflow 的问题
csv.field_size_limit(2**31 - 1)

ROOT = Path("./TrialRes/Trial_8")

GENERAL_RE = re.compile(
    r"general_distribution_(AdjList|AdjMatrix)_(DFS|BFS)_(\d+)_([0-9.]+)_(\d+)\.csv"
)
OPTIMAL_RE = re.compile(
    r"optimal_distribution_(AdjList|AdjMatrix)_(DFS|BFS)_(\d+)_([0-9.]+)_(\d+)\.csv"
)

def _split_items(items_field: str):
    """
    items 字段格式： "rank1 | rank2 | rank3 ..."
    rank 格式： "1;2;8;0;...;4"
    """
    if not items_field:
        return set()
    s = items_field.strip()
    if not s:
        return set()
    parts = [p.strip() for p in s.split(" | ") if p.strip()]
    return set(parts)

def load_agg_distribution(path: Path):
    """
    读取聚合分布文件：
      key,count,items
      4,50,"r1 | r2 | ..."

    返回：
      spaces: dict[int, int]     # space -> count
      ranks_all: set[str]        # 所有 rank 字符串（用于求交集）
    """
    spaces = {}
    ranks_all = set()

    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            if not row:
                continue

            key = row.get("key")
            count = row.get("count")
            items = row.get("items", "")

            if key is None or count is None:
                continue

            space = int(key)
            c = int(count)
            spaces[space] = spaces.get(space, 0) + c

            ranks_all |= _split_items(items)

    return spaces, ranks_all

def analyze_one_case(storage, traversal, n, p, number, general_path: Path, optimal_path: Path):
    general_spaces, general_ranks = load_agg_distribution(general_path)
    optimal_spaces, optimal_ranks = load_agg_distribution(optimal_path)

    sorted_spaces = sorted(general_spaces.keys())
    best_space = sorted_spaces[0] if len(sorted_spaces) >= 1 else None
    second_space = sorted_spaces[1] if len(sorted_spaces) >= 2 else None

    opt_rank_count = sum(optimal_spaces.values()) if optimal_spaces else 0
    hit_best_space = optimal_spaces.get(best_space, 0) if best_space is not None else 0
    hit_second_space = optimal_spaces.get(second_space, 0) if second_space is not None else 0

    found_in_general = len(optimal_ranks & general_ranks) if (optimal_ranks and general_ranks) else 0

    return {
        "storage": storage,
        "traversal": traversal,
        "n": n,
        "p": p,
        "number": number,
        "best_space": best_space,
        "opt_rank_count": opt_rank_count,
        "hit_best_space": hit_best_space,
        "second_space": second_space,
        "hit_second_space": hit_second_space,
        "found_in_general": found_in_general,
    }

def main():
    general_files = {}
    optimal_files = {}

    for f in ROOT.glob("general_distribution_*.csv"):
        m = GENERAL_RE.match(f.name)
        if m:
            general_files[m.groups()] = f

    for f in ROOT.glob("optimal_distribution_*.csv"):
        m = OPTIMAL_RE.match(f.name)
        if m:
            optimal_files[m.groups()] = f

    rows = []
    for key, gpath in general_files.items():
        opath = optimal_files.get(key)
        if opath is None:
            continue

        storage, traversal, n, p, number = key
        rows.append(
            analyze_one_case(
                storage=storage,
                traversal=traversal,
                n=int(n),
                p=float(p),
                number=number,
                general_path=gpath,
                optimal_path=opath,
            )
        )

    out_path = ROOT / "trial_8_summary_by_case.csv"
    with out_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "storage",
                "traversal",
                "n",
                "p",
                "number",
                "best_space",
                "opt_rank_count",
                "hit_best_space",
                "second_space",
                "hit_second_space",
                "found_in_general",
            ],
        )
        writer.writeheader()
        writer.writerows(rows)

    print(f"[trial_8_anlys] written: {out_path}")

if __name__ == "__main__":
    main()