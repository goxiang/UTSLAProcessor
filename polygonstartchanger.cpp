#include "polygonstartchanger.h"
#include "qmath.h"

namespace Utek {

PolygonStartChanger::PolygonStartChanger()
{

}

ReturnPathPack PolygonStartChanger::getStartChanger(const ClipperLib::Path &path, //封闭轮廓path,已经经过倍率缩放
                                                    const StartChangerPara &paras) const
{
    ReturnPathPack rPack;
    rPack.successFirst = false;
    rPack.successLast = false;

    if (path.size() < 3 || paras.changerType == 0 || paras.insertLength == 0.0f)
        return rPack;
    return getStartChangerPrivate(path, paras);
}

ReturnPathPack PolygonStartChanger::getStartChangerPrivate(const ClipperLib::Path &path, const StartChangerPara &paras) const
{
    ReturnPathPack rPack;
    rPack.successFirst = false;
    rPack.successLast = false;

    double fArea = ClipperLib::Area(path);
    int ori = (fArea >= 0) ? 1 : 0;

    if(paras.smallPolyLength > 0.0f)
    {
        if(getLength(path) < paras.smallPolyLength)
        {
            return rPack;
        }
    }
    if(paras.smallPolyArea > 0.0f)
    {
        if(qAbs(fArea) < paras.smallPolyArea)
        {
            return rPack;
        }
    }

    ClipperLib::IntPoint wInsertedPtFirst, wInsertedPtLast;
    bool fFindValidPtFirst = false, fFindValidPtLast = false;

    int nTotalPtCnt = path.size();
    int stepCount = 0;
    int nPrevIndex = 0;
    int nAfterIndex = 0;

    do {
        if(stepCount > nTotalPtCnt - 1)
        {
            break;
        }

        nPrevIndex = stepCount - 1;
        if(nPrevIndex < 0)
        {
            nPrevIndex = nTotalPtCnt - 1;
        }
        wInsertedPtFirst = getPossibleStartPt(path.at(stepCount),
                                              path.at(nPrevIndex),
                                              paras.insertAngle,
                                              paras.insertLength);

        int inOriPath = ClipperLib::PointInPolygon(wInsertedPtFirst, path);
        if ((ori == CONTOUR_OUTLINE && inOriPath != POINT_OUT_OF_CONTOUR) ||
            (ori == CONTOUR_INHOLE && inOriPath != POINT_IN_CONTOUR))
        {
            if(false == intersecting(path, stepCount, wInsertedPtFirst))
            {
                fFindValidPtFirst = true;
                break;
            }
        }
    }while(!fFindValidPtFirst && ++stepCount < stepTryMax);

    if (fFindValidPtFirst && paras.changerType == 2)
    {
        nAfterIndex = stepCount + 1;
        if(nAfterIndex > nTotalPtCnt - 1)
        {
            nAfterIndex = 0;
        }
        wInsertedPtLast = getPossibleStartPtRev(path.at(stepCount),
                                                path.at(nAfterIndex),
                                                paras.insertAngle,
                                                paras.insertLength);
        int inOriPath = ClipperLib::PointInPolygon(wInsertedPtLast, path);
        if ((ori == CONTOUR_OUTLINE && inOriPath != POINT_OUT_OF_CONTOUR) ||
            (ori == CONTOUR_INHOLE && inOriPath != POINT_IN_CONTOUR))
        {
            if(false == intersecting(path, stepCount, wInsertedPtLast))
            {
                fFindValidPtLast = true;
            }
        }
    }

    //优先首点
    if (fFindValidPtFirst)
    {
        rPack.successFirst = true;
        rPack.firstPt = wInsertedPtFirst;
        rPack.result = path;
        push(rPack.result, stepCount);

        if (fFindValidPtLast)
        {
            rPack.successLast = true;
            rPack.lastPt = wInsertedPtLast;
        }
    }

    return rPack;
}

float PolygonStartChanger::getLength(const ClipperLib::Path &path) const
{
    float len(0.0f);
    for (uint i = 0; i < path.size(); i++)
    {
        if (i == path.size() - 1)
            len += dis(path.at(i), path.at(0));
        else
            len += dis(path.at(i), path.at(i + 1));
    }
    return len;
}

float PolygonStartChanger::dis(const ClipperLib::IntPoint &p1, const ClipperLib::IntPoint &p2) const
{
    return qSqrt((p2.X - p1.X) * (p2.X - p1.X) + (p2.Y - p1.Y) * (p2.Y - p1.Y));
}

ClipperLib::IntPoint PolygonStartChanger::getPossibleStartPt(const ClipperLib::IntPoint &firstPt,
                                                             const ClipperLib::IntPoint &beforePt,
                                                             float startK,
                                                             float startDis) const
{
    Vector2d nVec = normalVector(beforePt, firstPt);
    nVec = unitVector(nVec);
    Vector2d beforeVec(firstPt.X - beforePt.X, firstPt.Y - beforePt.Y);

    //内外轮廓中，内部点始终在既有线段左侧，外部点始终在既有线段右侧，即叉乘始终为正
    if (crossProduct(beforeVec, nVec) < 0)
    {
        nVec.x = -nVec.x;
        nVec.y = -nVec.y;
    }

    Vector2d unitVecBefore = unitVector(beforeVec);
    Vector2d tVec(startK * unitVecBefore.x - nVec.x,
                  startK * unitVecBefore.y - nVec.y);
    float tl = qSqrt(startK * startK + 1);
    tVec.x /= tl;
    tVec.y /= tl;

    return ClipperLib::IntPoint(firstPt.X - tVec.x * startDis,
                                firstPt.Y - tVec.y * startDis);
}

ClipperLib::IntPoint PolygonStartChanger::getPossibleStartPtRev(const ClipperLib::IntPoint &firstPt,
                                                                const ClipperLib::IntPoint &beforePt,
                                                                float startK, float startDis) const
{
    Vector2d nVec = normalVector(beforePt, firstPt);
    nVec = unitVector(nVec);
    Vector2d beforeVec(firstPt.X - beforePt.X, firstPt.Y - beforePt.Y);

    if (crossProduct(beforeVec, nVec) >= 0)
    {
        nVec.x = -nVec.x;
        nVec.y = -nVec.y;
    }

    Vector2d unitVecBefore = unitVector(beforeVec);
    Vector2d tVec(startK * unitVecBefore.x - nVec.x,
                  startK * unitVecBefore.y - nVec.y);
    float tl = qSqrt(startK * startK + 1);
    tVec.x /= tl;
    tVec.y /= tl;

    return ClipperLib::IntPoint(firstPt.X - tVec.x * startDis,
                                firstPt.Y - tVec.y * startDis);
}

Vector2d PolygonStartChanger::normalVector(const ClipperLib::IntPoint &pt1, const ClipperLib::IntPoint &pt2) const
{
    return normalVector(pt1.X, pt1.Y, pt2.X, pt2.Y);
}

Vector2d PolygonStartChanger::normalVector(float x1, float y1, float x2, float y2) const
{
    if (IS_SAME(x1, x2) && IS_SAME(y1, y2))
        return Vector2d(0.0f, 0.0f);
    if (IS_SAME(x1, x2))
        return normalVector(INT_INF);
    float kOfLine = (y2 - y1) / (x2 - x1);
    return normalVector(kOfLine);
}

Vector2d PolygonStartChanger::normalVector(float k) const
{
    if (IS_ZERO(k)) return Vector2d(0.0f, 1.0f);
    if (IS_INF(k)) return Vector2d(1.0f, 0.0f);

    return Vector2d(1.0f , -1.0f / k);
}

Vector2d PolygonStartChanger::unitVector(const Vector2d &vec) const
{
    if (IS_ZERO(vec.x) && IS_ZERO(vec.y))
        return vec;
    float length = qSqrt(vec.x * vec.x + vec.y * vec.y);
    return Vector2d(vec.x / length, vec.y / length);
}

float PolygonStartChanger::crossProduct(const Vector2d &vec1, const Vector2d &vec2) const
{
    return (vec1.x * vec2.y - vec1.y * vec2.x);
}

void PolygonStartChanger::push(ClipperLib::Path &path, int moveStep) const
{
    if (moveStep >= path.size())
        moveStep %= path.size();
    if (moveStep == 0) return;

    ClipperLib::Path nPath;
    for (uint i = moveStep; i < path.size(); i++)
    {
        nPath.push_back(path.at(i));
    }
    for (int i = 0; i < moveStep; i++)
    {
        nPath.push_back(path.at(i));
    }

    path.swap(nPath);
}

bool PolygonStartChanger::intersecting(const ClipperLib::Path &path, const int &nIndex, const ClipperLib::IntPoint &pt) const
{
    int nPtCnt = path.size() - 1;
    if(nIndex < 0 || nIndex > nPtCnt)
    {
        return true;
    }

    ClipperLib::cInt minX, maxX, minY, maxY;

    ClipperLib::cInt facA = path.at(nIndex).X - pt.X;
    ClipperLib::cInt facB = path.at(nIndex).Y - pt.Y;

    ClipperLib::cInt facC = 0;
    ClipperLib::cInt facD = 0;

    if(path.at(nIndex).X > pt.X)
    {
        minX = pt.X;
        maxX = path.at(nIndex).X;
    }
    else
    {
        minX = path.at(nIndex).X;
        maxX = pt.X;
    }

    if(path.at(nIndex).Y > pt.Y)
    {
        minY = pt.Y;
        maxY = path.at(nIndex).Y;
    }
    else
    {
        minY = path.at(nIndex).Y;
        maxY = pt.Y;
    }

    int nPt = nIndex + 1;
    int nPtNext = 0;
    if(nPt > nPtCnt)
    {
        nPt = 0;
    }

    for(int iPt = 1; iPt < nPtCnt; ++ iPt, nPt = nPtNext)
    {
        nPtNext = nPt + 1;
        if(nPtNext > nPtCnt)
        {
            nPtNext = 0;
        }

        if(path.at(nPt).X < path.at(nPtNext).X)
        {
            if(path.at(nPt).X > maxX || path.at(nPtNext).X < minX)
            {
                continue;
            }
        }
        else
        {
            if(path.at(nPtNext).X > maxX || path.at(nPt).X < minX)
            {
                continue;
            }
        }

        if(path.at(nPt).Y < path.at(nPtNext).Y)
        {
            if(path.at(nPt).Y > maxY || path.at(nPtNext).Y < minY)
            {
                continue;
            }
        }
        else
        {
            if(path.at(nPtNext).Y > maxY || path.at(nPt).Y < minY)
            {
                continue;
            }
        }

        facC = path.at(nPtNext).X - path.at(nPt).X;
        facD = path.at(nPtNext).Y - path.at(nPt).Y;

        if((facA * (path.at(nPt).Y - pt.Y) > facB * (path.at(nPt).X - pt.X)) !=
            (facA * (path.at(nPtNext).Y - pt.Y) > facB * (path.at(nPtNext).X - pt.X)) ||
            (facC * (pt.Y - path.at(nPt).Y) > facD * (pt.X - path.at(nPt).X)) !=
                (facC * (path.at(nIndex).Y - path.at(nPt).Y) > facD * (path.at(nIndex).Y - path.at(nPt).Y)))
        {
            return true;
        }
    }
    return false;
}

}
