#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Trial_7 analysis

读取目录下所有：
- general_distribution_<n>_<p>_<number>.csv
  CSV 表头：key,count,items
  其中：
    key   = 空间占用（整数）
    count = 该空间占用下访问秩数量（整数）
    items = 一个超长字符串，内部用 " | " 拼接多个访问秩，每个访问秩形如 "6;5;1;4;3;2;0"

- graph_info_<n>_<p>_<number>.csv
  CSV 表头：node,degree,adjNodes
  adjNodes 形如：0;2;5

输出（默认输出到同目录，也可指定 --out）：
- best_ranks_<n>_<p>_<number>.csv
  每一个“最优 key（最小空间占用）”对应的访问秩单独占一行（去重、保序）
- graph_<n>_<p>_<number>.png
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import sys
from dataclasses import dataclass
from typing import List, Optional, Tuple

import matplotlib.pyplot as plt
import networkx as nx

# 允许超长 CSV 字段（items 那一列会非常长）
try:
    csv.field_size_limit(sys.maxsize)
except OverflowError:
    csv.field_size_limit(2**31 - 1)

_DIST_RE = re.compile(r"^general_distribution_(\d+)_([0-9.]+)_(\d+)\.csv$")
_INFO_RE = re.compile(r"^graph_info_(\d+)_([0-9.]+)_(\d+)\.csv$")


@dataclass(frozen=True)
class Params:
    n: int
    p: float
    number: int


def parse_params_from_filename(fname: str) -> Params:
    base = os.path.basename(fname)
    m = _DIST_RE.match(base)
    if not m:
        raise ValueError(f"invalid filename format: {base}")
    return Params(n=int(m.group(1)), p=float(m.group(2)), number=int(m.group(3)))


def dedup_preserve_order(items: List[str]) -> List[str]:
    seen = set()
    out: List[str] = []
    for it in items:
        if it in seen:
            continue
        seen.add(it)
        out.append(it)
    return out


def split_items_field(items: str) -> List[str]:
    # items 字段内部用 " | " 拼接访问秩
    # 可能没有空格，直接按 "|" split 最稳
    parts = [x.strip() for x in (items or "").split("|")]
    return [x for x in parts if x]


def read_best_ranks_from_general_distribution(csv_path: str) -> Tuple[int, int, List[str]]:
    """
    读取格式：
      key,count,items
      2,7056,"rank1 | rank2 | ..."

    规则：
    - 找 key 最小的行（可能有多行同最小 key）
    - best_count_sum 为这些行 count 的累加
    - ranks 为这些行 items 拆出来的 rank 合并后去重（保序）
    """
    best_key: Optional[int] = None
    best_count_sum: int = 0
    best_ranks: List[str] = []

    with open(csv_path, "r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        required = {"key", "count", "items"}
        if not required.issubset(set(reader.fieldnames or [])):
            raise ValueError(f"invalid header in {os.path.basename(csv_path)}: {reader.fieldnames}")

        for row in reader:
            key = int((row.get("key") or "").strip())
            cnt = int((row.get("count") or "").strip())
            items = row.get("items") or ""

            ranks = split_items_field(items)

            if best_key is None or key < best_key:
                best_key = key
                best_count_sum = cnt
                best_ranks = list(ranks)
            elif key == best_key:
                best_count_sum += cnt
                best_ranks.extend(ranks)

    if best_key is None:
        raise ValueError(f"no valid rows in: {csv_path}")

    best_ranks = dedup_preserve_order(best_ranks)
    return best_key, best_count_sum, best_ranks


def write_best_ranks_csv(
    out_path: str,
    params: Params,
    best_space: int,
    best_count_sum: int,
    ranks: List[str],
) -> None:
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(["n", "p", "number", "best_space", "best_count_sum", "rank"])
        for r in ranks:
            w.writerow([params.n, f"{params.p:.6f}", params.number, best_space, best_count_sum, r])


def read_graph_info_to_nx(csv_path: str) -> nx.Graph:
    """
    graph_info CSV：
      node,degree,adjNodes
      6,3,0;2;5
    """
    G = nx.Graph()

    with open(csv_path, "r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        required = {"node", "degree", "adjNodes"}
        if not required.issubset(set(reader.fieldnames or [])):
            raise ValueError(f"invalid header in {os.path.basename(csv_path)}: {reader.fieldnames}")

        for row in reader:
            node = int((row.get("node") or "").strip())
            G.add_node(node)

            adj = (row.get("adjNodes") or "").strip()
            if not adj:
                continue

            for s in adj.split(";"):
                s = s.strip()
                if not s:
                    continue
                nei = int(s)
                if nei != node:
                    G.add_edge(node, nei)

    return G


def draw_graph(G: nx.Graph, out_path: str, title: str) -> None:
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    plt.figure(figsize=(8, 6))
    pos = nx.spring_layout(G, seed=42)
    nx.draw_networkx(
        G,
        pos=pos,
        with_labels=True,
        node_size=600,
        font_size=10,
        width=1.5,
    )
    plt.title(title)
    plt.axis("off")
    plt.tight_layout()
    plt.savefig(out_path, dpi=200)
    plt.close()


def process_one(dist_path: str, in_dir: str, out_dir: str, verbose: bool) -> None:
    params = parse_params_from_filename(os.path.basename(dist_path))

    best_space, best_count_sum, ranks = read_best_ranks_from_general_distribution(dist_path)

    out_best = os.path.join(out_dir, f"best_ranks_{params.n}_{params.p:.6f}_{params.number}.csv")
    write_best_ranks_csv(out_best, params, best_space, best_count_sum, ranks)

    info_name = f"graph_info_{params.n}_{params.p:.6f}_{params.number}.csv"
    info_path = os.path.join(in_dir, info_name)
    if os.path.exists(info_path):
        G = read_graph_info_to_nx(info_path)
        out_png = os.path.join(out_dir, f"graph_{params.n}_{params.p:.6f}_{params.number}.png")
        draw_graph(G, out_png, title=f"n={params.n}, p={params.p:.6f}, number={params.number}")

    if verbose:
        print(
            f"[ok] {os.path.basename(dist_path)} -> {os.path.basename(out_best)} "
            f"(best_space={best_space}, best_count_sum={best_count_sum}, unique_ranks={len(ranks)})"
        )


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default=".", help="输入目录（包含 general_distribution_*.csv / graph_info_*.csv）")
    ap.add_argument("--out", default=None, help="输出目录（默认与 --dir 相同）")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    in_dir = args.dir
    out_dir = args.out or in_dir

    if not os.path.isdir(in_dir):
        print(f"[error] input directory not found: {in_dir}")
        return 1

    dist_files = [f for f in os.listdir(in_dir) if _DIST_RE.match(f)]
    if args.verbose:
        print(f"[info] in_dir = {os.path.abspath(in_dir)}")
        print(f"[info] out_dir = {os.path.abspath(out_dir)}")
        print(f"[info] matched general_distribution files: {len(dist_files)}")

    ok = 0
    fail = 0
    for fname in dist_files:
        try:
            process_one(os.path.join(in_dir, fname), in_dir=in_dir, out_dir=out_dir, verbose=args.verbose)
            ok += 1
        except Exception as e:
            fail += 1
            print(f"[error] failed on {fname}: {e}")

    print(f"[done] processed={ok}, failed={fail}, out_dir={os.path.abspath(out_dir)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())