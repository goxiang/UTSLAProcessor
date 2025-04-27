#ifndef SCANLINESSORTOR_H
#define SCANLINESSORTOR_H

#include "bpccommon.h"

#include "quadtreesortor.h"

#include <QSharedPointer>

struct SCANLINE;

///
/// ! @coreclass{ScanLinesSortor}
/// 扫描线排序器,独立类
///
class ScanLinesSortor
{
public:
    ScanLinesSortor(const int &, const int &);

    void sortScanLines(QVector<SCANLINE> &);
    void sortPaths(Paths &);

    template<class T>
    static void sortDatas(QVector<T> &);

private:
    struct Priv;
    QSharedPointer<Priv> d = nullptr;
};

template<class T>
void ScanLinesSortor::sortDatas(QVector<T> &datas)
{
    QTreeSortor sortor;
    sortor.sortDatas<T>(datas);
}


#endif // SCANLINESSORTOR_H
