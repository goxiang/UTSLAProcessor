#include "quadtreesortor.h"
#include "quadtreedef.h"

#include <QElapsedTimer>

struct QuadtreeSortor::Priv {
    void calcSortLineVec(const Paths &paths, MQuadTree &quadtree, MPtInfoPtr &firstPt)
    {
        /*
        uint totalSize = paths.size();
        int startIndex = 0;
        int curStep = 0;

        int curIndex = 0;
        int nextIndex = 0;

        while (startIndex < paths.size())
        {
            curStep = 0;

        LABEL_LOOP:
            curIndex = startIndex + curStep;
            auto &path = paths[curIndex];
            if(path.size() < 2) {
                ++ startIndex;
                continue;
            }

            nextIndex = startIndex + curStep + 1;
            if (nextIndex < totalSize)
            {
                auto &nextPath = paths[nextIndex];
                if(nextPath.size() > 1)
                {
                    if (fabs(path.back().X - nextPath.front().X) + fabs(path.back().Y - nextPath.front().Y) < 2)
                    {
                        ++ curStep;
                        goto LABEL_LOOP;
                    }
                }
            }
            addTreeNode(quadtree, startIndex, startIndex + curStep,
                        paths[startIndex].front(), paths[startIndex + curStep].back(), firstPt);
            startIndex = startIndex + curStep + 1;
        }
        /*/
        for(uint i = 0; i < paths.size(); ++ i) {
            auto &path = paths[i];
            if(path.size() < 2) continue;
            auto pt1 = MPtInfoPtr(new MPtInfo(path.front().X, path.front().Y, i, 0));
            auto pt2 = MPtInfoPtr(new MPtInfo(path.back().X, path.back().Y, i, 1));
            pt1->_bro = pt2;
            pt2->_bro = pt1;
            quadtree.insertNode(pt1);
            quadtree.insertNode(pt2);
            if (0 == i) firstPt = pt1;
        }//*/
    }
    void addTreeNode(MQuadTree &quadtree, const int &index1, const int &index2,
                     const IntPoint &pt1, const IntPoint &pt2, MPtInfoPtr &firstPt)
    {
        auto ptPtr1 = MPtInfoPtr(new MPtInfo(pt1.X, pt1.Y, index1, 0));
        auto ptPtr2 = MPtInfoPtr(new MPtInfo(pt2.X, pt2.Y, index2, 1));
        ptPtr1->_bro = ptPtr2;
        ptPtr2->_bro = ptPtr1;
        quadtree.insertNode(ptPtr1);
        quadtree.insertNode(ptPtr2);
        // tempPtrSet.insert(ptPtr2);
        if (nullptr == firstPt) firstPt = ptPtr1;
    }

    QSet<MPtInfoPtr> tempPtrSet;
};

QuadtreeSortor::QuadtreeSortor() :
    d(new Priv) {

}

void QuadtreeSortor::sortPaths(Paths &paths)
{
    MQuadTree quadtree;
    MPtInfoPtr firstPt = nullptr;

    // Paths srcPaths = std::move(paths);
    // paths.clear();
    // 计算排序线向量
    d->calcSortLineVec(paths, quadtree, firstPt);
    // qDebug() << "QuadtreeSortor::sort - 1" << timer.elapsed() << quadtree.getNodeCount() << paths.size();

    Paths desPaths;
    // return;
    // 使用四叉树进行路径排序
    quadtree.sortPaths(firstPt, paths, desPaths);
    paths = std::move(desPaths);
    // qDebug() << "QuadtreeSortor::sort - 2" << timer.elapsed();
}
