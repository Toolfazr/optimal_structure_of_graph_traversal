#pragma once

#include <string>
#include "Graph.hpp"
#include "GenCmd.hpp"

class Generator {
public:
    virtual ~Generator() = default;
    virtual bool parseCmd(GenCmd cmd);
    virtual std::unique_ptr<Graph> genGraph();
};