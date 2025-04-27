#ifndef ALGORITHMCHECKER_H
#define ALGORITHMCHECKER_H

#include <QJsonObject>
#include <QList>
#include "bpccommon.h"

///
/// ! @coreclass{AlgorithmChecker}
/// 用于路径规划的验证和控制,独立类
///
class AlgorithmChecker
{
public:
    void createCheckers(const QJsonObject &, const int &platWidth,  const int &platHeight);
    inline void changeLayerOddInfo() { bOddLayer = !bOddLayer; }
    inline double getRotateAngle() { return fRotateAngle; }

    inline bool scanCheckerBorder() { return nScanCheckerBorder; }
    QList<Paths>  &getCheckerList() {
        if(nOffsetChecker) {
            if(bOddLayer) return checkerListOffset;
            else return checkerList;
        }
        else return checkerList;
    }

private:
    void createOverlapCheckers(const int &, const int &, const int &platWidth,  const int &platHeight,
                               const int &overlapWid, const int &offsetChecker);

private:
    bool bOddLayer = false;

    int nOffsetChecker = 0;
    int nScanCheckerBorder = 0;
    double fRotateAngle = 0.0;
    QList<Paths> checkerList;
    QList<Paths> checkerListOffset;
};

#endif // ALGORITHMCHECKER_H
