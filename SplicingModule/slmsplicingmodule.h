#ifndef SLMSPLICINGMODULE_H
#define SLMSPLICINGMODULE_H

#include <QObject>
#include <QSharedPointer>

#include "utbpprocessbase.h"
#include "splicingheader.h"

struct PARAWRITEBUFF;
struct WriterBufferParas;
struct BPCUSTOMERPARAS;
struct UFFWRITEDATA;
struct LayerSplicingArea;
struct ScannerSplicingArea;

class SLMSplicingModulePriv;

typedef std::function<void(const int &, const Paths &)> FuncSplit;

///
/// ! @coreclass{SLMSplicingModule}
/// 实现多区域协同打印的关键类
///
class SLMSplicingModule
{
public:
    SLMSplicingModule();

    void initializeParas(const WriterBufferParas *);
    void calcScannerArea();

    void recalcSplicingArea(Paths &, QList<AREAINFOPTR> &, const QList<AREAINFOPTR> &solidPaths = QList<AREAINFOPTR>());
    void addLayerSplicingArea(const int &);

    void splitSupportData(const LayerSplicingArea &, const Paths &, FuncSplit &&) const;
    void splitBorderData(const LayerSplicingArea &, const Paths &, FuncSplit &&, const bool &,
                         const std::function<void(Paths &)> &&) const;
    void splitSolidData(const LayerSplicingArea &, const Paths &, FuncSplit &&) const;

    inline QSharedPointer<ScannerSplicingArea> getSplicingArea() const;
    LayerSplicingArea getLayerSplicing(const int &) const;

private:
    QSharedPointer<SLMSplicingModulePriv> d = nullptr;
};

#endif // SLMSPLICINGMODULE_H
