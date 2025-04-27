#ifndef ALGORITHMSTRIP_H
#define ALGORITHMSTRIP_H

#include <QJsonObject>
#include "bpccommon.h"

///
/// ! @coreclass{AlgorithmStrip}
/// 负责生成条带式填充路径,独立类
class AlgorithmStrip
{
public:
    AlgorithmStrip() = default;

    void createStrips(const QJsonObject &jsonObj, const int &platWidth, const int &platHeight);
    void createStrips(const int &nHei, const int &platWidth, const int &platHeight, const int &overlapWid);
    Paths getStripList(const double &, double &hatchingAngle, const int &);

private:
    Paths pathsTemp;
    int nOffsetX = 0;
    int nOffsetY = 0;
};

#endif // ALGORITHMSTRIP_H
