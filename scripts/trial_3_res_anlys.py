#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Analyze a large CSV (millions of rows) with schema:
label,bfsMaxQueue,dfsMaxStack,bfsHDS,dfsHDS,bfsBS,dfsBS

For each label:
- Define "optimal" as the minimum bfsMaxQueue / dfsMaxStack (smaller peak memory is better).
- Compute ratio P(metric_opt < metric_nonopt) for DFS and BFS, for BOTH BS and HDS:
    ratio = (#(x in opt, y in nonopt) with x < y) / (|opt| * |nonopt|)
  Computed efficiently with sorting + searchsorted (no O(n^2) loops).
- Report metric ranges (min/max) for opt group and non-opt group.
"""

import argparse
import csv
import math
import os
import sys
from array import array
from typing import Dict, Tuple, Optional

import numpy as np


def _to_int(s: str) -> int:
    return int(s.strip())


def _to_float(s: str) -> float:
    return float(s.strip())


def _clean_label(s: str) -> str:
    # keep internal spaces, just strip ends
    return s.strip()


def prob_x_lt_y(x: np.ndarray, y: np.ndarray) -> float:
    """
    Strict probability P(X < Y) where X and Y are 1D float arrays.
    Returns NaN if either is empty.
    """
    if x.size == 0 or y.size == 0:
        return float("nan")
    y_sorted = np.sort(y, kind="quicksort")
    # for each x, count y > x  == len(y) - upper_bound(y, x)
    ub = np.searchsorted(y_sorted, x, side="right")
    greater = y_sorted.size - ub
    return float(greater.sum() / (x.size * y_sorted.size))


def update_minmax(cur_min: Optional[float], cur_max: Optional[float], v: float) -> Tuple[float, float]:
    if cur_min is None:
        return v, v
    return (v if v < cur_min else cur_min), (v if v > cur_max else cur_max)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv_path", help="Input CSV path")
    ap.add_argument("-o", "--out", default="summary_by_label.csv", help="Output summary CSV path")
    ap.add_argument("--encoding", default="utf-8", help="Input file encoding (default: utf-8)")
    ap.add_argument("--delimiter", default=",", help="CSV delimiter (default: ,)")
    args = ap.parse_args()

    csv_path = args.csv_path
    if not os.path.exists(csv_path):
        print(f"ERROR: file not found: {csv_path}", file=sys.stderr)
        sys.exit(2)

    # PASS 1: find per-label optimal (minimum) bfsMaxQueue and dfsMaxStack
    min_bfs: Dict[str, int] = {}
    min_dfs: Dict[str, int] = {}

    with open(csv_path, "r", encoding=args.encoding, newline="") as f:
        reader = csv.DictReader(f, delimiter=args.delimiter)
        required = [
            "label",
            "bfsMaxQueue",
            "dfsMaxStack",
            "bfsBS",
            "dfsBS",
            "bfsHDS",
            "dfsHDS",
        ]
        for col in required:
            if col not in reader.fieldnames:
                print(f"ERROR: missing column '{col}', got columns: {reader.fieldnames}", file=sys.stderr)
                sys.exit(2)

        for row in reader:
            label = _clean_label(row["label"])
            bfs_q = _to_int(row["bfsMaxQueue"])
            dfs_s = _to_int(row["dfsMaxStack"])

            mb = min_bfs.get(label)
            if mb is None or bfs_q < mb:
                min_bfs[label] = bfs_q

            md = min_dfs.get(label)
            if md is None or dfs_s < md:
                min_dfs[label] = dfs_s

    # PASS 2: collect metrics (BS + HDS) into opt/non-opt per label and track ranges/counts
    # BS
    dfs_opt_bs: Dict[str, array] = {}
    dfs_non_bs: Dict[str, array] = {}
    bfs_opt_bs: Dict[str, array] = {}
    bfs_non_bs: Dict[str, array] = {}

    # HDS
    dfs_opt_hds: Dict[str, array] = {}
    dfs_non_hds: Dict[str, array] = {}
    bfs_opt_hds: Dict[str, array] = {}
    bfs_non_hds: Dict[str, array] = {}

    # min/max trackers: dict[label] -> (min,max)
    dfs_opt_mm: Dict[str, Tuple[Optional[float], Optional[float]]] = {}
    dfs_non_mm: Dict[str, Tuple[Optional[float], Optional[float]]] = {}
    bfs_opt_mm: Dict[str, Tuple[Optional[float], Optional[float]]] = {}
    bfs_non_mm: Dict[str, Tuple[Optional[float], Optional[float]]] = {}

    dfs_opt_hds_mm: Dict[str, Tuple[Optional[float], Optional[float]]] = {}
    dfs_non_hds_mm: Dict[str, Tuple[Optional[float], Optional[float]]] = {}
    bfs_opt_hds_mm: Dict[str, Tuple[Optional[float], Optional[float]]] = {}
    bfs_non_hds_mm: Dict[str, Tuple[Optional[float], Optional[float]]] = {}

    # counts (opt / non-opt by space peak)
    dfs_opt_n: Dict[str, int] = {}
    dfs_non_n: Dict[str, int] = {}
    bfs_opt_n: Dict[str, int] = {}
    bfs_non_n: Dict[str, int] = {}

    with open(csv_path, "r", encoding=args.encoding, newline="") as f:
        reader = csv.DictReader(f, delimiter=args.delimiter)
        for row in reader:
            label = _clean_label(row["label"])
            bfs_q = _to_int(row["bfsMaxQueue"])
            dfs_s = _to_int(row["dfsMaxStack"])
            bfs_bs = _to_float(row["bfsBS"])
            dfs_bs = _to_float(row["dfsBS"])
            bfs_hds = _to_float(row["bfsHDS"])
            dfs_hds = _to_float(row["dfsHDS"])

            # DFS split by optimal dfsMaxStack
            if dfs_s == min_dfs[label]:
                # BS
                arr = dfs_opt_bs.get(label)
                if arr is None:
                    arr = array("d")
                    dfs_opt_bs[label] = arr
                arr.append(dfs_bs)
                dfs_opt_n[label] = dfs_opt_n.get(label, 0) + 1
                mn, mx = dfs_opt_mm.get(label, (None, None))
                dfs_opt_mm[label] = update_minmax(mn, mx, dfs_bs)
                # HDS
                arrh = dfs_opt_hds.get(label)
                if arrh is None:
                    arrh = array("d")
                    dfs_opt_hds[label] = arrh
                arrh.append(dfs_hds)
                mn, mx = dfs_opt_hds_mm.get(label, (None, None))
                dfs_opt_hds_mm[label] = update_minmax(mn, mx, dfs_hds)
            else:
                # BS
                arr = dfs_non_bs.get(label)
                if arr is None:
                    arr = array("d")
                    dfs_non_bs[label] = arr
                arr.append(dfs_bs)
                dfs_non_n[label] = dfs_non_n.get(label, 0) + 1
                mn, mx = dfs_non_mm.get(label, (None, None))
                dfs_non_mm[label] = update_minmax(mn, mx, dfs_bs)
                # HDS
                arrh = dfs_non_hds.get(label)
                if arrh is None:
                    arrh = array("d")
                    dfs_non_hds[label] = arrh
                arrh.append(dfs_hds)
                mn, mx = dfs_non_hds_mm.get(label, (None, None))
                dfs_non_hds_mm[label] = update_minmax(mn, mx, dfs_hds)

            # BFS split by optimal bfsMaxQueue
            if bfs_q == min_bfs[label]:
                # BS
                arr = bfs_opt_bs.get(label)
                if arr is None:
                    arr = array("d")
                    bfs_opt_bs[label] = arr
                arr.append(bfs_bs)
                bfs_opt_n[label] = bfs_opt_n.get(label, 0) + 1
                mn, mx = bfs_opt_mm.get(label, (None, None))
                bfs_opt_mm[label] = update_minmax(mn, mx, bfs_bs)
                # HDS
                arrh = bfs_opt_hds.get(label)
                if arrh is None:
                    arrh = array("d")
                    bfs_opt_hds[label] = arrh
                arrh.append(bfs_hds)
                mn, mx = bfs_opt_hds_mm.get(label, (None, None))
                bfs_opt_hds_mm[label] = update_minmax(mn, mx, bfs_hds)
            else:
                # BS
                arr = bfs_non_bs.get(label)
                if arr is None:
                    arr = array("d")
                    bfs_non_bs[label] = arr
                arr.append(bfs_bs)
                bfs_non_n[label] = bfs_non_n.get(label, 0) + 1
                mn, mx = bfs_non_mm.get(label, (None, None))
                bfs_non_mm[label] = update_minmax(mn, mx, bfs_bs)
                # HDS
                arrh = bfs_non_hds.get(label)
                if arrh is None:
                    arrh = array("d")
                    bfs_non_hds[label] = arrh
                arrh.append(bfs_hds)
                mn, mx = bfs_non_hds_mm.get(label, (None, None))
                bfs_non_hds_mm[label] = update_minmax(mn, mx, bfs_hds)

    # Compute ratios and write summary
    out_cols = [
        "label",
        "dfs_opt_dfsMaxStack",
        "dfs_opt_count",
        "dfs_nonopt_count",
        "dfs_ratio_P(optBS_lt_nonoptBS)",
        "dfs_optBS_min",
        "dfs_optBS_max",
        "dfs_nonoptBS_min",
        "dfs_nonoptBS_max",
        "dfs_ratio_P(optHDS_lt_nonoptHDS)",
        "dfs_optHDS_min",
        "dfs_optHDS_max",
        "dfs_nonoptHDS_min",
        "dfs_nonoptHDS_max",
        "bfs_opt_bfsMaxQueue",
        "bfs_opt_count",
        "bfs_nonopt_count",
        "bfs_ratio_P(optBS_lt_nonoptBS)",
        "bfs_optBS_min",
        "bfs_optBS_max",
        "bfs_nonoptBS_min",
        "bfs_nonoptBS_max",
        "bfs_ratio_P(optHDS_lt_nonoptHDS)",
        "bfs_optHDS_min",
        "bfs_optHDS_max",
        "bfs_nonoptHDS_min",
        "bfs_nonoptHDS_max",
    ]

    labels = sorted(set(min_bfs.keys()) | set(min_dfs.keys()))

    with open(args.out, "w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=out_cols)
        w.writeheader()

        for label in labels:
            # DFS: BS
            x_dfs = np.frombuffer(dfs_opt_bs.get(label, array("d")), dtype=np.float64)
            y_dfs = np.frombuffer(dfs_non_bs.get(label, array("d")), dtype=np.float64)
            dfs_ratio = prob_x_lt_y(x_dfs, y_dfs)
            dfs_opt_min, dfs_opt_max = dfs_opt_mm.get(label, (None, None))
            dfs_non_min, dfs_non_max = dfs_non_mm.get(label, (None, None))

            # DFS: HDS
            x_dfs_h = np.frombuffer(dfs_opt_hds.get(label, array("d")), dtype=np.float64)
            y_dfs_h = np.frombuffer(dfs_non_hds.get(label, array("d")), dtype=np.float64)
            dfs_hds_ratio = prob_x_lt_y(x_dfs_h, y_dfs_h)
            dfs_opt_h_min, dfs_opt_h_max = dfs_opt_hds_mm.get(label, (None, None))
            dfs_non_h_min, dfs_non_h_max = dfs_non_hds_mm.get(label, (None, None))

            # BFS: BS
            x_bfs = np.frombuffer(bfs_opt_bs.get(label, array("d")), dtype=np.float64)
            y_bfs = np.frombuffer(bfs_non_bs.get(label, array("d")), dtype=np.float64)
            bfs_ratio = prob_x_lt_y(x_bfs, y_bfs)
            bfs_opt_min, bfs_opt_max = bfs_opt_mm.get(label, (None, None))
            bfs_non_min, bfs_non_max = bfs_non_mm.get(label, (None, None))

            # BFS: HDS
            x_bfs_h = np.frombuffer(bfs_opt_hds.get(label, array("d")), dtype=np.float64)
            y_bfs_h = np.frombuffer(bfs_non_hds.get(label, array("d")), dtype=np.float64)
            bfs_hds_ratio = prob_x_lt_y(x_bfs_h, y_bfs_h)
            bfs_opt_h_min, bfs_opt_h_max = bfs_opt_hds_mm.get(label, (None, None))
            bfs_non_h_min, bfs_non_h_max = bfs_non_hds_mm.get(label, (None, None))

            w.writerow({
                "label": label,

                "dfs_opt_dfsMaxStack": min_dfs.get(label, ""),
                "dfs_opt_count": dfs_opt_n.get(label, 0),
                "dfs_nonopt_count": dfs_non_n.get(label, 0),
                "dfs_ratio_P(optBS_lt_nonoptBS)": "" if math.isnan(dfs_ratio) else dfs_ratio,
                "dfs_optBS_min": "" if dfs_opt_min is None else dfs_opt_min,
                "dfs_optBS_max": "" if dfs_opt_max is None else dfs_opt_max,
                "dfs_nonoptBS_min": "" if dfs_non_min is None else dfs_non_min,
                "dfs_nonoptBS_max": "" if dfs_non_max is None else dfs_non_max,

                "dfs_ratio_P(optHDS_lt_nonoptHDS)": "" if math.isnan(dfs_hds_ratio) else dfs_hds_ratio,
                "dfs_optHDS_min": "" if dfs_opt_h_min is None else dfs_opt_h_min,
                "dfs_optHDS_max": "" if dfs_opt_h_max is None else dfs_opt_h_max,
                "dfs_nonoptHDS_min": "" if dfs_non_h_min is None else dfs_non_h_min,
                "dfs_nonoptHDS_max": "" if dfs_non_h_max is None else dfs_non_h_max,

                "bfs_opt_bfsMaxQueue": min_bfs.get(label, ""),
                "bfs_opt_count": bfs_opt_n.get(label, 0),
                "bfs_nonopt_count": bfs_non_n.get(label, 0),
                "bfs_ratio_P(optBS_lt_nonoptBS)": "" if math.isnan(bfs_ratio) else bfs_ratio,
                "bfs_optBS_min": "" if bfs_opt_min is None else bfs_opt_min,
                "bfs_optBS_max": "" if bfs_opt_max is None else bfs_opt_max,
                "bfs_nonoptBS_min": "" if bfs_non_min is None else bfs_non_min,
                "bfs_nonoptBS_max": "" if bfs_non_max is None else bfs_non_max,

                "bfs_ratio_P(optHDS_lt_nonoptHDS)": "" if math.isnan(bfs_hds_ratio) else bfs_hds_ratio,
                "bfs_optHDS_min": "" if bfs_opt_h_min is None else bfs_opt_h_min,
                "bfs_optHDS_max": "" if bfs_opt_h_max is None else bfs_opt_h_max,
                "bfs_nonoptHDS_min": "" if bfs_non_h_min is None else bfs_non_h_min,
                "bfs_nonoptHDS_max": "" if bfs_non_h_max is None else bfs_non_h_max,
            })

    print(f"Wrote: {args.out}")
    print(f"Labels: {len(labels)}")
    print("Note: 'optimal' is defined as MIN dfsMaxStack / MIN bfsMaxQueue per label.")


if __name__ == "__main__":
    main()
