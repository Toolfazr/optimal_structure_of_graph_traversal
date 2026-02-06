#pragma once

#include "Iterator.hpp"

class Aggregate {
public:
    virtual Iterator& iterator() = 0;
};