#include "DistributionStorage.hpp"
#include <fstream>
#include <sstream>

using namespace std;

void DistributionStorage::clear()
{
    distribution.clear();
    accessRankNum = 0;
}

const std::map<size_t, std::vector<std::vector<string>>> &DistributionStorage::getDistribution()
{
    return distribution;
}

unsigned long long DistributionStorage::size()
{
    return accessRankNum;
}

void DistributionStorage::insert(vector<string> accessRank, size_t maxSize)
{
    distribution[maxSize].push_back(accessRank);
    accessRankNum += 1;
}

void DistributionStorage::toCsv(const std::string &path) const
{
    std::ofstream ofs(path);
    if (!ofs.is_open())
    {
        throw std::runtime_error("Failed to open csv file: " + path);
    }

    // header
    ofs << "key,count,items\n";

    for (const auto &[key, buckets] : distribution)
    {
        ofs << key << "," << buckets.size() << ",\"";

        bool firstBucket = true;
        for (const auto &bucket : buckets)
        {
            if (!firstBucket)
                ofs << " | ";
            firstBucket = false;

            bool firstItem = true;
            for (const auto &item : bucket)
            {
                if (!firstItem)
                    ofs << ";";
                firstItem = false;
                ofs << item;
            }
        }

        ofs << "\"\n";
    }

    ofs.close();
}