#ifndef ALGORITHMDISTRIBUTION_H
#define ALGORITHMDISTRIBUTION_H

#include <QDebug>
#include <memory>
#include <vector>
#include <set>
#include <map>

struct SegementsInfo {
    int _index = -1;
    std::vector<double> _rateVec;
    double getTotalRate() {
        double totalWeight = 0.0;
        for (const auto &weight : _rateVec) totalWeight += weight;
        return totalWeight;
    }
    SegementsInfo() = default;
    SegementsInfo(const int &index, const double &rate) {
        _rateVec.clear();
        _index = index;
        _rateVec.push_back(rate);
    }
};

struct PartInfo {
    int _index = -1;
    double _weight = 0.0;
    double _factor = 1.0;
    double _realWeight = 0.0;

    PartInfo(const int &index, const double &weight,
             const double &factor = 1.0) {
        _index = index;
        _weight = weight;
        _factor = factor;
        _realWeight = _weight * _factor;
    }
};

struct DistributionResult {
    std::vector<PartInfo> _partsList;
    double _totalWeight = 0.0;
};

struct PartResult {
    int _containorIndex = 1;
    double _factor = 1.0;

    PartResult(const int &index, const double &factor) {
        _containorIndex = index;
        _factor = factor;
    }
};

inline QDebug operator<<(QDebug debug, const PartResult &partRes)
{
    debug << "PartResult" << "{ ";
    debug << partRes._containorIndex << " " << partRes._factor;
    debug << " } ";
    return debug;
}


enum DistAreaType {
    Border = 0,
    Support,
    Hatching,
    Downface,
    Upface
};

struct DistPartInfo {
    int _index = -1;
    double _weight = 0.0;

    DistAreaType _distAreaType = DistAreaType::Border;
    std::set<int> _allowedContainer = {};
    std::vector<PartResult> _distResultVec = {};

    explicit DistPartInfo(const int &index, const double &weight,
                          const DistAreaType &distAreaType, const std::set<int> &allowedVec) {
        _index = index;
        _weight = weight;
        _distAreaType = distAreaType;
        _allowedContainer = allowedVec;
    }
    explicit DistPartInfo(const int &index, const double &weight,
                          const DistAreaType &distAreaType, const int &containorIndex) {
        _index = index;
        _weight = weight;
        _distAreaType = distAreaType;
        _allowedContainer = {containorIndex};
    }

    inline void insertResult(const int &index, const double &factor) {
        _distResultVec.push_back(PartResult(index, factor));
    }
};

class AlgorithmDistribution
{
public:
    AlgorithmDistribution() = default;

public:
    void distribution(std::vector<DistributionResult> &, const std::vector<double> &inVec,
                      const int &containerCnt = 2, const double &tolerance = 1E-1);
    void distribution(std::vector<DistributionResult> &, const std::map<int, double> &inMap,
                      const int &containerCnt = 2, const double &tolerance = 1E-1);
    void distribution(std::map<int, std::vector<PartResult>> &, const std::map<int, double> &inMap,
                      const int &containerCnt = 2, const double &tolerance = 1E-1);

    void getPartResultMap(const std::vector<DistributionResult> &, std::map<int, std::vector<PartResult>> &partResultMap);


    static void distribution(std::vector<std::shared_ptr<DistPartInfo>> &, std::string *output_msg = nullptr);
};

#endif // ALGORITHMDISTRIBUTION_H
