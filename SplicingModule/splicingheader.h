#ifndef SPLICINGHEADER_H
#define SPLICINGHEADER_H

#include <QSharedPointer>
#include <QVector>
#include <QMap>
#include <QSet>
#include "bpccommon.h"

#include "splicingabstract.h"

struct ScanFieldInfo {
    Path scanField;
    IntRect scanRc;
    Paths independentPaths;
    IntPoint center;
    QMap<int, QSharedPointer<SplicingAbstract>> splicingMap;
};

struct CoincideFiledInfo {
    QSet<int> scannerSet;
    Paths subPaths;
};

struct SplicingAreaInfo {
    int scannerIndex = 0;
    Paths layerScanField;
    IntRect rectField;
};

typedef QSharedPointer<SplicingAreaInfo> SplicingAreaPtr;

struct LayerSplicingArea {
    QMap<int, SplicingAreaPtr> splicingAreaMap_support;
    QMap<int, SplicingAreaPtr> splicingAreaMap_border;
    QMap<int, SplicingAreaPtr> splicingAreaMap_solid;
};

struct ScannerSplicingArea {
    QMap<int, ScanFieldInfo> scanFiledMap;
    QVector<CoincideFiledInfo> coincideFieldVec;
    QVector<QSharedPointer<SplicingAbstract>> splicingPtrVec;

    LayerSplicingArea layerSplicingArea;
};
#endif // SPLICINGHEADER_H
