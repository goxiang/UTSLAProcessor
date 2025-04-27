#include "waterdistributionpriv.h"
#include "algorithmdistribution.h"

#include <algorithm>
#include <QDebug>

//
std::vector<DistributionResult> WaterDistributionPriv::initDistribution(const std::map<int, double> &inMap,
                                                                        const int &containerCnt,
                                                                        const double &tolerance)
{
    std::vector<DistributionResult> res;
    _distributionNum = containerCnt < 1 ? 1 : containerCnt;
    _tolerance = tolerance;
    std::vector<PartInfo> mateVec;
    _averageWeight = createMateVec(inMap, mateVec, _distributionNum);
    sortWeightVec(mateVec);

    startDistribution(res, mateVec, containerCnt, 0);
    return res;
}

void WaterDistributionPriv::startDistribution(std::vector<DistributionResult> &res,
                                              std::vector<PartInfo> &mateVec,
                                              const int &containerCnt,
                                              const int &startPos)
{
    if (containerCnt > 1)
    {
        int leftContainerCnt_1 = containerCnt >> 1;
        int leftContainerCnt_2 = leftContainerCnt_1;

        double fRate1 = 0.5, fRate2 = 0.5;

        if (containerCnt & 0x1)
        {
            leftContainerCnt_2 = leftContainerCnt_1 + 1;

            fRate1 = double(leftContainerCnt_1) / (containerCnt);
            fRate2 = 1 - fRate1;
        }
        std::vector<PartInfo> out1, out2;
        segementParts(mateVec, out1, out2, fRate1, fRate2, startPos + leftContainerCnt_1);
        startDistribution(res, out1, leftContainerCnt_1, startPos);
        startDistribution(res, out2, leftContainerCnt_2, startPos + leftContainerCnt_1);
    }
    else
    {
        DistributionResult rst;
        rst._partsList = mateVec;
        updateResInfo(rst, 0);
        res.push_back(std::move(rst));
    }
}

///private
double WaterDistributionPriv::createMateVec(const std::map<int, double> &inMap,
                                            std::vector<PartInfo> &mateVec, const int &distributionNum)
{
    double totalWeight = 0.0;
    for (const auto &inInfo : inMap)
    {
        totalWeight += inInfo.second;
        mateVec.push_back(PartInfo(inInfo.first, inInfo.second));
        if (inInfo.first < 0) _maxTotalWeight = std::max(_maxTotalWeight, inInfo.second);
    }
    totalWeight = totalWeight / distributionNum;

    return totalWeight;
}

void WaterDistributionPriv::sortWeightVec(std::vector<PartInfo> &mateVec)
{
    std::sort(mateVec.begin(), mateVec.end(),
              [](const PartInfo &v1, const PartInfo &v2) -> bool {
                  return v1._realWeight > v2._realWeight;
              });
}

void WaterDistributionPriv::segementParts(std::vector<PartInfo> &inArray, std::vector<PartInfo> &out1,
                                          std::vector<PartInfo> &out2, const double &rate1, const double &rate2,
                                          const int &containerCnt)
{
    if (inArray.size() < 1) return;

    out1.clear();
    out2.clear();

    std::sort(inArray.begin(), inArray.end(), [](const PartInfo &v1, const PartInfo &v2) -> bool {
        if (v1._index < 0 && v2._index < 0) return v1._realWeight > v2._realWeight;
        if (v1._index < 0) return true;
        if (v2._index < 0) return false;
        return v1._realWeight > v2._realWeight;
    });

    double factor = rate2 / rate1;
    double targetWeight1 = 0.0;
    for (const auto &partInfo : inArray) targetWeight1 += partInfo._realWeight;
    targetWeight1 *= rate1;

    DistributionResult res1, res2;
    int nArraySize = inArray.size();

    int distributionIndex = 0;
    while (true)
    {
        if (distributionIndex >= inArray.size())
        {
            distributionIndex = -1;
            break;
        }
        if (inArray[distributionIndex]._index >= 0) break;

        if (abs(inArray[distributionIndex]._index) <= containerCnt) res1 << std::move(inArray[distributionIndex]);
        else res2 << std::move(inArray[distributionIndex]);
        ++ distributionIndex;
    }
    if (distributionIndex < 0) goto LABEL_END;

    if (inArray[distributionIndex]._realWeight < _allowedSegementArea ||
        inArray[distributionIndex]._realWeight < _maxTotalWeight)
    {
        for (int i = distributionIndex; i < nArraySize; ++ i)
        {
            if (res1._totalWeight * factor <= res2._totalWeight) res1 << std::move(inArray[i]);
            else res2 << std::move(inArray[i]);
        }
    }
    else
    {
        // Sample Distribution
        for (int i = distributionIndex + 1; i < nArraySize; ++ i)
        {
            if (i == distributionIndex) continue;
            if (res1._totalWeight * factor <= res2._totalWeight) res1 << std::move(inArray[i]);
            else res2 << std::move(inArray[i]);
        }

        // Calculate Segement
        auto segementRate1 = (targetWeight1 - res1._totalWeight) / inArray[distributionIndex]._realWeight;
        if (segementRate1 < 1E-6 || (segementRate1 * inArray[distributionIndex]._realWeight) < _allowedMinArea)
        {
            res2 << std::move(inArray[distributionIndex]);
        }
        else if (fabs(1 - segementRate1) < 1E-6 || (fabs(1 - segementRate1) * inArray[distributionIndex]._realWeight) < _allowedMinArea)
        {
            res1 << std::move(inArray[distributionIndex]);
        }
        else
        {
            res1 << PartInfo(inArray[distributionIndex]._index, inArray[distributionIndex]._weight,
                             inArray[distributionIndex]._factor * segementRate1);
            res2 << PartInfo(inArray[distributionIndex]._index, inArray[distributionIndex]._weight,
                             inArray[distributionIndex]._factor * (1 - segementRate1));
        }
    }

LABEL_END:
    out1 = std::move(res1._partsList);
    out2 = std::move(res2._partsList);
}
