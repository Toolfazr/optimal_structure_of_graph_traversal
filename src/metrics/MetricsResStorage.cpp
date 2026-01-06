#include "MetricsResStorage.hpp"

const std::vector<RootOptResult>& MetricsResStorage::getDfsMaxStack() const {
    return this->dfsMaxStack;
}
const std::vector<RootOptResult>& MetricsResStorage::getBfsMaxQueue() const {
    return this->bfsMaxQueue;
}
const std::vector<double>& MetricsResStorage::getBfsHDS() const {
    return this->bfsHDS;
}
const std::vector<double>& MetricsResStorage::getDfsHDS() const {
    return this->dfsHDS;
}

const std::vector<double>& MetricsResStorage::getBfsBS() const {
    return this->bfsBS;
}

const std::vector<double>& MetricsResStorage::getDfsBS() const {
    return this->dfsBS;
}

size_t MetricsResStorage::getResGroupSize() const {
    return resGroupSize;
}

bool MetricsResStorage::append(
    RootOptResult _bfsMaxQueue,
    RootOptResult _dfsMaxStack,
    double _bfsHDS,
    double _dfsHDS,
    double _bfsBS,
    double _dfsBS,
    std::string _label) {
    resGroupSize = this->bfsBS.size();
    if(
        resGroupSize != this->dfsBS.size() ||
        resGroupSize != this->bfsHDS.size() ||
        resGroupSize != this->dfsHDS.size() ||
        resGroupSize != this->bfsMaxQueue.size() ||
        resGroupSize != this->dfsMaxStack.size() ||
        resGroupSize != this->label.size()
        ) {
            return false;
        }

    this->bfsBS.push_back(_bfsBS);
    this->dfsBS.push_back(_dfsBS);
    this->dfsHDS.push_back(_dfsHDS);
    this->bfsHDS.push_back(_bfsHDS);
    this->dfsMaxStack.push_back(_dfsMaxStack);
    this->bfsMaxQueue.push_back(_bfsMaxQueue);
    this->label.push_back(_label);

    resGroupSize += 1;
    return true;
}

void MetricsResStorage::merge(MetricsResStorage& another) {
    if (&another == this) return;

    if (another.resGroupSize == 0) return;

    if (another.dfsMaxStack.size() != another.resGroupSize ||
        another.bfsMaxQueue.size() != another.resGroupSize ||
        another.bfsHDS.size()     != another.resGroupSize ||
        another.dfsHDS.size()     != another.resGroupSize ||
        another.bfsBS.size()      != another.resGroupSize ||
        another.dfsBS.size()      != another.resGroupSize ||
        another.label.size() != another.resGroupSize) {
            return;
    }

    const size_t add = another.resGroupSize;

    dfsMaxStack.reserve(resGroupSize + add);
    bfsMaxQueue.reserve(resGroupSize + add);
    bfsHDS.reserve(resGroupSize + add);
    dfsHDS.reserve(resGroupSize + add);
    bfsBS.reserve(resGroupSize + add);
    dfsBS.reserve(resGroupSize + add);
    label.reserve(resGroupSize + add);

    dfsMaxStack.insert(dfsMaxStack.end(), another.dfsMaxStack.begin(), another.dfsMaxStack.end());
    bfsMaxQueue.insert(bfsMaxQueue.end(), another.bfsMaxQueue.begin(), another.bfsMaxQueue.end());
    bfsHDS.insert(bfsHDS.end(), another.bfsHDS.begin(), another.bfsHDS.end());
    dfsHDS.insert(dfsHDS.end(), another.dfsHDS.begin(), another.dfsHDS.end());
    bfsBS.insert(bfsBS.end(), another.bfsBS.begin(), another.bfsBS.end());
    dfsBS.insert(dfsBS.end(), another.dfsBS.begin(), another.dfsBS.end());
    label.insert(label.end(), another.label.begin(), another.label.end());

    resGroupSize += add;
}

void MetricsResStorage::clear() {
    resGroupSize = 0;

    dfsMaxStack.clear();
    bfsMaxQueue.clear();
    bfsHDS.clear();
    dfsHDS.clear();
    bfsBS.clear();
    dfsBS.clear();
    label.clear();
}

const std::vector<std::string>& MetricsResStorage::getLabel() const{
    return label;
}