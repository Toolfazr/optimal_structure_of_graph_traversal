#include "RandomGen.hpp"
#include <regex>
#include "AdjListGraph.hpp"
#include "AdjMatrixGraph.hpp"
#include "GenCmd.hpp"

RandomGen::RandomGen() {
    factory["AdjList"] = [this]() {
        return makeGraph<AdjListGraph>(n, p);
    };

    factory["AdjMatrix"] = [this]() {
        return makeGraph<AdjMatrixGraph>(n, p);
    };
}

bool RandomGen::parseCmd(GenCmd cmd) {
    static const std::regex re(R"(^RandomGraph ([A-Za-z/]+) (\d+) (\d+(?:\.\d+)?)$)");
    std::string str = cmd.getCmd();
    std::smatch matcher;

    if(!std::regex_match(str, matcher, re)) return false;

    structureType = matcher[1].str();
    if(factory.find(structureType) == factory.end()) return false;

    n = std::stoul(matcher[2].str());
    p = std::stod(matcher[3].str());

    if(n <= 0) return false;
    if(p == 0.0 && n > 1) return false;
    if(p < 0.0 || p > 1.0) return false;

    return true;
}

std::unique_ptr<Graph> RandomGen::genGraph() {
    auto it = factory.find(structureType);
    if (it == factory.end())
    {
        return nullptr;
    }
    return it->second();
}