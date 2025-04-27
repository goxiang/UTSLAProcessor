#ifndef SPLICING_MODULE_PRIV_HHH_HH_H
#define SPLICING_MODULE_PRIV_HHH_HH_H

#include "splicingheader.h"

///
/// ! @coreclass{SLMSplicingModulePriv}
/// 拼接模块的私有实现类
///
class SLMSplicingModulePriv
{
public:
    SLMSplicingModulePriv();

public:
    void calcLinesOnRect(const IntRect &, const Paths &, Paths &);
    void removeLineOnRect(const IntRect &, Paths &);

private:
    void calcLinesOnRect(const IntRect &, const Path &, Paths &);
    bool removeLineOnRect(const IntRect &, const Path &, Paths &);
    bool clipSectionOnVerticalLine(const int64_t &x, const int64_t &minY, const int64_t &maxY, IntPoint &pt1, IntPoint &pt2);
    bool clipSectionOnHorizontalLine(const int64_t &y, const int64_t &minX, const int64_t &maxX, IntPoint &pt1, IntPoint &pt2);

private:
    friend class SLMSplicingModule;
    QSharedPointer<ScannerSplicingArea> _scannerSplicing;
    QMap<int, LayerSplicingArea> _layerSplicingMap;
};

#endif //SPLICING_MODULE_PRIV_HHH_HH_H
