#ifndef QUADTREESORTOR_H
#define QUADTREESORTOR_H

#include "quadtreedef.h"
#include "bpccommon.h"

#include <QSharedPointer>

///
/// ! @coreclass{QuadtreeSortor}
/// 用于路径优化的排序组件,独立类
///
class QuadtreeSortor
{
public:
    QuadtreeSortor();

    void sortPaths(Paths &);

private:
    struct Priv;
    QSharedPointer<Priv> d = nullptr;
};


class QTreeSortor
{
public:
    QTreeSortor() = default;
    template<class T>
    void sortDatas(QVector<T> &);
};

template<class T>
void QTreeSortor::sortDatas(QVector<T> &datas)
{
    MQuadTree quadtree;
    MPtInfoPtr firstPt = nullptr;

    for(uint i = 0; i < datas.size(); ++ i) {
        auto &data = datas[i];
        auto pt1 = MPtInfoPtr(new MPtInfo(data._centerX, data._centerY, i, 0));
        auto pt2 = MPtInfoPtr(new MPtInfo(data._centerX, data._centerY, i, 1));
        pt1->_bro = pt2;
        pt2->_bro = pt1;
        quadtree.insertNode(pt1);
        quadtree.insertNode(pt2);
        if (0 == i) firstPt = pt1;
    }

    QVector<T> desVec;
    quadtree.sortPaths(firstPt, datas, desVec);
    datas = std::move(desVec);
}

#endif // QUADTREESORTOR_H
