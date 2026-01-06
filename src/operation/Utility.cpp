#include "Utility.hpp"
#include "Metrics.hpp"
#include <fstream>
#include <iomanip>
#include <filesystem>

namespace {
    static std::string csvEscape(const std::string& s) {
        bool needQuote = false;
        for (char c : s) {
            if (c == ',' || c == '"' || c == '\n' || c == '\r') { needQuote = true; break; }
        }
        if (!needQuote) return s;
    
        std::string out;
        out.reserve(s.size() + 2);
        out.push_back('"');
        for (char c : s) out += (c == '"') ? "\"\"" : std::string(1, c);
        out.push_back('"');
        return out;
    }

    static bool fileIsEmptyOrMissing(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open()) return true;
        ifs.seekg(0, std::ios::end);
        return ifs.tellg() == 0;
    }
} // anonymous namespace

void Utility::extractGraphInfo(
    const Graph& graph,
    std::vector<std::vector<Index>>& originalAdj,
    std::vector<Node>& originalNodes
) {
    originalAdj.clear();
    originalNodes.clear();

    int n = static_cast<int>(graph.getNodeCount());
    originalAdj.resize(n);
    originalNodes.resize(n, Node(-1, "none"));

    for (int id = 0; id < n; ++id) {
        originalAdj[id] = graph.getNeighbors(id);
        originalNodes[id] = graph.getNode(id);
    }
}

MetricsResStorage Utility::doSpaceMeasure(Graph& graph) {
    RootOptResult dfsMaxStack = Metrics::measureDFSMaxStack(graph);
    RootOptResult bfsMaxQueue = Metrics::measureBFSMaxQueue(graph);
    double bfsHDS = Metrics::measureHighDegreeSpacing(graph, TraversalAlgo::bfsTrace);
    double dfsHDS = Metrics::measureHighDegreeSpacing(graph, TraversalAlgo::dfsTrace);
    double bfsBS  = Metrics::measureBranchSuspension(graph, TraversalAlgo::bfsTrace);
    double dfsBS  = Metrics::measureBranchSuspension(graph, TraversalAlgo::dfsTrace);

    MetricsResStorage res;
    res.append(bfsMaxQueue, dfsMaxStack, bfsHDS, dfsHDS ,bfsBS, dfsBS, graph.getLabel());
    return res;
}

void Utility::saveRes(const std::string& path, const MetricsResStorage& res) {
    const bool needHeader = fileIsEmptyOrMissing(path);

    std::ofstream ofs(path, std::ios::out | std::ios::app);
    if (!ofs.is_open()) return;

    if (needHeader) {
        ofs << "label,bfsMaxQueue,dfsMaxStack,bfsHDS,dfsHDS,bfsBS,dfsBS\n";
    }
    ofs << std::setprecision(17);

    const size_t n = res.getResGroupSize();
    const auto& labels = res.getLabel();
    const auto& bfsQ   = res.getBfsMaxQueue();
    const auto& dfsS   = res.getDfsMaxStack();
    const auto& bfsH   = res.getBfsHDS();
    const auto& dfsH   = res.getDfsHDS();
    const auto& bfsB   = res.getBfsBS();
    const auto& dfsB   = res.getDfsBS();

    // 防御性检查：不一致直接不写
    if (labels.size() != n || bfsQ.size() != n || dfsS.size() != n ||
        bfsH.size()   != n || dfsH.size() != n || bfsB.size() != n || dfsB.size() != n) {
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        ofs << csvEscape(labels[i]) << ','
            << bfsQ[i].bestPeak << ','
            << dfsS[i].bestPeak << ','
            << bfsH[i] << ','
            << dfsH[i] << ','
            << bfsB[i] << ','
            << dfsB[i] << '\n';
    }

    ofs.flush();
}