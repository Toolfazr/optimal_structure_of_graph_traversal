import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

distribution_path_1 = "./TrialRes/Trial_6/general_distribution_list_dfs.csv"
distribution_path_2 = "./TrialRes/Trial_6/optimal_distribution_list_dfs.csv"

output_path = "./TrialRes/Trial_6/distribution_compare_list_dfs.png"

# 读取数据
df1 = pd.read_csv(distribution_path_1)
df2 = pd.read_csv(distribution_path_2)

# 合并 key
all_keys = sorted(set(df1["key"]).union(set(df2["key"])))

map1 = dict(zip(df1["key"], df1["count"]))
map2 = dict(zip(df2["key"], df2["count"]))

counts1 = [map1.get(k, 0) for k in all_keys]
counts2 = [map2.get(k, 0) for k in all_keys]

x = np.arange(len(all_keys))
width = 0.4

plt.figure(figsize=(12, 6))
plt.bar(x - width / 2, counts1, width=width, label="general")
plt.bar(x + width / 2, counts2, width=width, label="optimal")

plt.xlabel("distribution key")
plt.ylabel("count")
plt.xticks(x, all_keys, rotation=45)
plt.legend()
plt.tight_layout()

plt.savefig(output_path, dpi=300)
plt.close()