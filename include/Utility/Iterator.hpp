#pragma once

class Iterator {
public:
    virtual bool hasNext() = 0;
    virtual void* next() = 0;
};