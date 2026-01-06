# center_low_high_demo.py
# pip install networkx matplotlib

import networkx as nx
import matplotlib.pyplot as plt

def build_graph(n_low=6, n_high=3, high_degree=5):
    """
    一个中心节点(0)，连接若干低度节点(low)和若干高度节点(high)。
    - low 节点：只连接中心节点（度=1）
    - high 节点：连接中心节点 + 自己的若干“卫星节点”（从而度更高）
    """
    G = nx.Graph()
    center = 0
    G.add_node(center, role="center")

    # 低度节点
    low_nodes = []
    for i in range(1, 1 + n_low):
        G.add_node(i, role="low")
        G.add_edge(center, i)
        low_nodes.append(i)

    # 高度节点 + 卫星节点
    high_nodes = []
    next_id = 1 + n_low
    for _ in range(n_high):
        h = next_id
        next_id += 1
        G.add_node(h, role="high")
        G.add_edge(center, h)
        high_nodes.append(h)

        # 给每个 high 节点挂一些卫星节点，让它的度数更高
        for _ in range(high_degree - 1):  # 已经连了 center，所以再补 high_degree-1 条边
            s = next_id
            next_id += 1
            G.add_node(s, role="satellite")
            G.add_edge(h, s)

    return G

def draw_graph(G, title="Center + Low-degree + High-degree"):
    # 简单布局：spring layout
    pos = nx.spring_layout(G, seed=7)

    roles = nx.get_node_attributes(G, "role")
    node_list = list(G.nodes())

    # 用不同 marker size 区分（不指定颜色，避免你后续改配色不方便）
    sizes = []
    for v in node_list:
        r = roles.get(v, "")
        if r == "center":
            sizes.append(900)
        elif r == "high":
            sizes.append(520)
        elif r == "low":
            sizes.append(260)
        else:  # satellite
            sizes.append(140)

    nx.draw_networkx_edges(G, pos, width=1.5, alpha=0.7)
    nx.draw_networkx_nodes(G, pos, node_size=sizes)
    nx.draw_networkx_labels(G, pos, font_size=9)

    plt.title(title)
    plt.axis("off")
    plt.tight_layout()
    plt.savefig('demo_pic_1.png')

if __name__ == "__main__":
    G = build_graph(n_low=7, n_high=3, high_degree=6)
    draw_graph(G, title="Center(0) with low-degree and high-degree neighbors")
