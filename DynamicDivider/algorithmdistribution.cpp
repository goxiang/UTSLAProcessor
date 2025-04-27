#include "algorithmdistribution.h"
#include "TargetedDistribution/targeteddistribution.h"
#include "WaterDistribution/waterdistributionpriv.h"

#include <QDebug>

void AlgorithmDistribution::distribution(std::vector<DistributionResult> &res,
                                         const std::vector<double> &inVec,
                                         const int &containerCnt, const double &tolerance)
{
    std::map<int, double> inMap;
    auto index = 0;
    for (const auto &weight : inVec) inMap[index ++] = weight;

    WaterDistributionPriv algo;
    res = std::move(algo.initDistribution(inMap, containerCnt, tolerance));
}

void AlgorithmDistribution::distribution(std::vector<DistributionResult> &res,
                                         const std::map<int, double> &inMap,
                                         const int &containerCnt, const double &tolerance)
{
    WaterDistributionPriv algo;
    res = std::move(algo.initDistribution(inMap, containerCnt, tolerance));
}

void AlgorithmDistribution::distribution(std::map<int, std::vector<PartResult>> &partResultMap,
                                         const std::map<int, double> &inMap,
                                         const int &containerCnt, const double &tolerance)
{
    WaterDistributionPriv algo;
    auto res = algo.initDistribution(inMap, containerCnt, tolerance);
    // qDebug() << "res" << res;
    getPartResultMap(res, partResultMap);
}

void AlgorithmDistribution::getPartResultMap(const std::vector<DistributionResult> &resultVec,
                                             std::map<int, std::vector<PartResult>> &partResultMap)
{
    auto index = 0;
    for (const auto &distributionRes : resultVec)
    {
        for (const auto &partInfo : distributionRes._partsList)
        {
            partResultMap[partInfo._index].push_back(PartResult(index, partInfo._factor));
        }
        ++ index;
    }
}

void AlgorithmDistribution::distribution(std::vector<std::shared_ptr<DistPartInfo>> &partInfoVec, std::string *output_msg)
{
    TargetedDLib::TargetedDistribution targetDistribution;
    targetDistribution.distribution(partInfoVec, output_msg);
}
