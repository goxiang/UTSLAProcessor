#ifndef WATERDISTRIBUTIONPRIV_H
#define WATERDISTRIBUTIONPRIV_H

#include <QDebug>
#include <vector>
#include "./DynamicDivider/publicheader.h"

class WaterDistributionPriv
{
public:
    WaterDistributionPriv() = default;
    std::vector<DistributionResult> initDistribution(const std::map<int, double> &inMap,
                                                     const int &containerCnt, const double &tolerance);
    void startDistribution(std::vector<DistributionResult> &, std::vector<PartInfo> &,
                           const int &, const int &);

private:
    double createMateVec(const std::map<int, double> &inMap, std::vector<PartInfo> &, const int &);
    void sortWeightVec(std::vector<PartInfo> &mateVec);
    void segementParts(std::vector<PartInfo> &inArray, std::vector<PartInfo> &out1,
                       std::vector<PartInfo> &out2, const double &, const double &, const int &);

    double _distributionNum = 1;
    double _tolerance = 0.0;
private:
    double _averageWeight = 0.0;
    double _maxTotalWeight = 0.0;
    double _allowedSegementArea = 2.5E7;
    double _allowedMinArea = 2E6;
    // double _allowedSegementArea = 0;
    // double _allowedMinArea = 0;
};

#endif // WATERDISTRIBUTIONPRIV_H
