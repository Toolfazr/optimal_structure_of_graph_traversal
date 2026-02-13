#pragma once

#include <string>
#include "Graph.hpp"
#include <memory>

class GenCmd;

class GenCmd;

class Generator {
public:
    virtual ~Generator();
    virtual bool parseCmd(GenCmd cmd);
    virtual std::unique_ptr<Graph> genGraph();
};