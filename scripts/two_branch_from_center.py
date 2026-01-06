# two_branches_pretty.py
# pip install networkx matplotlib
# 可选(强烈推荐)：pip install pygraphviz  （否则自动回退到 spring_layout）

import networkx as nx
import matplotlib.pyplot as plt


def add_tree_branch(G: nx.Graph, attach_to: int, next_id: int, depth: int, fanout: int):
    current_level = [attach_to]
    for _ in range(depth):
        new_level = []
        for parent in current_level:
            for _ in range(fanout):
                child = next_id
                next_id += 1
                G.add_edge(parent, child)
                new_level.append(child)
        current_level = new_level
    return next_id


def build_graph(depth_left=4, fanout_left=2, depth_right=3, fanout_right=3):
    G = nx.Graph()
    c, a, b = 0, 1, 2
    G.add_edges_from([(c, a), (c, b)])  # center degree == 2

    next_id = 3
    next_id = add_tree_branch(G, attach_to=a, next_id=next_id, depth=depth_left, fanout=fanout_left)
    next_id = add_tree_branch(G, attach_to=b, next_id=next_id, depth=depth_right, fanout=fanout_right)
    return G, c, a, b


def hierarchy_pos(G: nx.Graph, root: int, left_root: int, right_root: int):
    """
    把整张图当作“中心 root + 两棵树”来排版：
    - root 在中间
    - left_root 的子树放左侧
    - right_root 的子树放右侧
    这样不会挤成一团，适合你这个“两个大分支”结构。
    """
    pos = {root: (0.0, 0.0), left_root: (-1.0, 0.0), right_root: (1.0, 0.0)}

    def place_subtree(start: int, sign: float):
        # BFS 分层，按层摆放；同层均匀铺开
        levels = {}
        parent = {start: root}
        q = [(start, 0)]
        seen = {root, start}
        while q:
            v, d = q.pop(0)
            levels.setdefault(d, []).append(v)
            for nb in G.neighbors(v):
                if nb in seen:
                    continue
                seen.add(nb)
                parent[nb] = v
                q.append((nb, d + 1))

        # 每一层：x 方向随深度远离中心，y 方向分散
        for d, nodes in levels.items():
            # x：向左右展开
            x = sign * (1.0 + 1.25 * d)
            # y：同层节点均匀分布，避免重叠
            m = len(nodes)
            if m == 1:
                ys = [0.0]
            else:
                span = max(1.0, 0.55 * m)  # 节点越多，拉得越开
                ys = [(-span / 2) + i * (span / (m - 1)) for i in range(m)]
            for v, y in zip(nodes, ys):
                # 轻微抖动：让边更好看一点（不需要随机）
                pos[v] = (x, y + 0.08 * d)

    place_subtree(left_root, sign=-1.0)
    place_subtree(right_root, sign=+1.0)
    return pos


def draw_graph(G: nx.Graph, c: int, a: int, b: int, title: str):
    # 1) 优先用 dot（树图最清晰）；没装 pygraphviz 就用手写分层布局
    pos = None
    try:
        pos = nx.nx_agraph.graphviz_layout(G, prog="dot")
        # dot 的方向默认是自上而下，这里做一次简单的坐标缩放/翻转让它更“横向两边展开”
        # 如果你喜欢 dot 的纵向树形，可以删掉下面这段变换。
        xs = [pos[v][0] for v in G.nodes()]
        ys = [pos[v][1] for v in G.nodes()]
        xmid = (min(xs) + max(xs)) / 2.0
        ymid = (min(ys) + max(ys)) / 2.0
        pos = {v: ((pos[v][0] - xmid) / 140.0, (pos[v][1] - ymid) / 140.0) for v in G.nodes()}
    except Exception:
        pos = hierarchy_pos(G, root=c, left_root=a, right_root=b)

    # 2) 节点大小分层；其余节点统一，画面更干净
    node_sizes = []
    for v in G.nodes():
        if v == c:
            node_sizes.append(950)
        elif v in (a, b):
            node_sizes.append(700)
        else:
            node_sizes.append(240)

    plt.figure(figsize=(11, 7))
    nx.draw_networkx_edges(G, pos, width=1.6, alpha=0.55)
    nx.draw_networkx_nodes(G, pos, node_size=node_sizes, linewidths=0.8)
    nx.draw_networkx_labels(G, pos, font_size=8)

    plt.title(title, pad=12)
    plt.axis("off")
    plt.tight_layout()
    plt.savefig('demo_pic_2.png')


if __name__ == "__main__":
    G, c, a, b = build_graph(depth_left=4, fanout_left=2, depth_right=3, fanout_right=3)
    draw_graph(G, c, a, b, title="Center degree=2 with two large branches (clean layout)")
