#include "splicingmodulepriv.h"
#include "bpccommon.h"

#include <QDebug>

#define DELTA_ERROR 5

SLMSplicingModulePriv::SLMSplicingModulePriv() :
    _scannerSplicing(new ScannerSplicingArea)
{
}

void SLMSplicingModulePriv::calcLinesOnRect(const IntRect &rect, const Paths &paths, Paths &onRectLineVec)
{
    onRectLineVec.clear();
    for(const auto &path : paths)
    {
        calcLinesOnRect(rect, path, onRectLineVec);
    }
}

void SLMSplicingModulePriv::removeLineOnRect(const IntRect &rect, Paths &paths)
{
    Paths des;
    for(auto &path : paths)
    {
        Paths desPaths;
        if(removeLineOnRect(rect, path, desPaths)) des << desPaths;
    }
    paths = des;
}

void SLMSplicingModulePriv::calcLinesOnRect(const IntRect &rect, const Path &path, Paths &onRectLineVec)
{
    auto ptCnt = path.size();
    if(ptCnt < 2) return;

    size_t index = 0;
    size_t nextIndex = 0;

    int64_t delta_x_1 = 0;
    int64_t delta_y_1 = 0;
    int64_t delta_x_2 = 0;
    int64_t delta_y_2 = 0;

    while(index < ptCnt)
    {
        auto pt1 = path[index ++];

        delta_x_1 = pt1.X - rect.left;
        delta_y_1 = pt1.Y - rect.top;
        if(abs(delta_x_1) < DELTA_ERROR && abs(delta_y_1) < DELTA_ERROR) continue;

        nextIndex = (index >= ptCnt) ? 0 : index;
        auto pt2 = path[nextIndex];

        if(pt1.X == pt2.X && pt1.Y == pt2.Y) continue;

        delta_x_2 = pt2.X - rect.left;
        delta_y_2 = pt2.Y - rect.top;

        if(abs(delta_x_1) < DELTA_ERROR && abs(delta_x_2) < DELTA_ERROR && delta_x_1 * delta_x_2 <= 0)
        {
            if(clipSectionOnVerticalLine(rect.left, rect.top, rect.bottom, pt1, pt2))
            {
                onRectLineVec << (Path() << pt1 << pt2);
            }
            continue;
        }
        else if(abs(delta_y_1) < DELTA_ERROR && abs(delta_y_2) < DELTA_ERROR && delta_y_1 * delta_y_2 <= 0)
        {
            if(pt1.X == pt2.X) continue;
            if(clipSectionOnHorizontalLine(rect.top, rect.left, rect.right, pt1, pt2))
            {
                onRectLineVec << (Path() << pt1 << pt2);
            }
        }
    }
}

bool SLMSplicingModulePriv::removeLineOnRect(const IntRect &rect, const Path &path, Paths &desPaths)
{
    auto ptCnt = path.size();
    if(ptCnt < 2) return false;

    Path tempPath;

    size_t index = 0;
    size_t nextIndex = 0;
    auto calcOnLineType = [](const IntPoint &pt, const IntRect &rc) -> int {
        int type = 0;
        if (fabs(pt.X - rc.left) < 3) type |= 0x1;
        if (fabs(pt.X - rc.right) < 3) type |= 0x10;
        if (fabs(pt.Y - rc.top) < 3) type |= 0x100;
        if (fabs(pt.Y - rc.bottom) < 3) type |= 0x1000;
        return type;
    };

    auto onLineType = 0;
    while(index < ptCnt)
    {
        auto &pt1 = path[index];
        ++ index;
        nextIndex = (index >= ptCnt) ? 0 : index;

        onLineType = calcOnLineType(pt1, rect);
        if(0 == onLineType)
        {
            tempPath << pt1;
            if(0 == nextIndex)
            {
                tempPath << path[0];
                desPaths << tempPath;
            }
            continue;
        }
        auto &pt2 = path[nextIndex];
        if(pt1.X == pt2.X && pt1.Y == pt2.Y) continue;
        tempPath << pt1;

        if (onLineType & calcOnLineType(pt2, rect))
        {
            if(tempPath.size() > 1) desPaths << tempPath;
            tempPath.clear();
        }
        else if(0 == nextIndex)
        {
            tempPath << pt2;
            desPaths << tempPath;
        }
    }
    return desPaths.size();
}

bool SLMSplicingModulePriv::clipSectionOnVerticalLine(const int64_t &x, const int64_t &minY, const int64_t &maxY, IntPoint &pt1, IntPoint &pt2)
{
    if(pt1.Y <= minY && pt2.Y <= minY) return false;
    if(pt1.Y >= maxY && pt2.Y >= maxY) return false;
    if(pt1.Y == pt2.Y) return false;

    if(pt1.X < x)
    {
        auto deltaY = int64_t(double(x - pt1.X) * (pt2.Y - pt1.Y) / (pt2.X - pt1.X) + pt1.Y);
        pt1.X = x;
        pt1.Y = deltaY;

    }
    else if(pt2.X < x)
    {
        auto deltaY = int64_t(double(x - pt1.X) * (pt2.Y - pt1.Y) / (pt2.X - pt1.X) + pt1.Y);
        pt2.X = x;
        pt2.Y = deltaY;
    }
    if(pt1.Y < pt2.Y)
    {
        if(pt1.Y < minY)
        {
            auto deltaX = int64_t(double(minY - pt1.Y) * (pt2.X - pt1.X) / (pt2.Y - pt1.Y) + pt1.X);
            pt1.Y = minY;
            pt1.X = deltaX;
        }
        if(pt2.Y > maxY)
        {
            auto deltaX = int64_t(double(maxY - pt1.Y) * (pt2.X - pt1.X) / (pt2.Y - pt1.Y) + pt1.X);
            pt2.Y = maxY;
            pt2.X = deltaX;
        }
    }
    else
    {
        if(pt2.Y < minY)
        {
            auto deltaX = int64_t(double(minY - pt1.Y) * (pt2.X - pt1.X) / (pt2.Y - pt1.Y) + pt1.X);
            pt2.Y = minY;
            pt2.X = deltaX;
        }
        if(pt1.Y > maxY)
        {
            auto deltaX = int64_t(double(maxY - pt1.Y) * (pt2.X - pt1.X) / (pt2.Y - pt1.Y) + pt1.X);
            pt1.Y = maxY;
            pt1.X = deltaX;
        }
    }
    return (pt1.X == x && pt2.X == x);
}

bool SLMSplicingModulePriv::clipSectionOnHorizontalLine(const int64_t &y, const int64_t &minX, const int64_t &maxX, IntPoint &pt1, IntPoint &pt2)
{
    if(pt1.X <= minX && pt2.X <= minX) return false;
    if(pt1.X >= maxX && pt2.X >= maxX) return false;
    if(pt1.X ==  pt2.X) return false;

    if(pt1.Y < y)
    {
        auto deltaX = int64_t(double(y - pt1.Y) * (pt2.X - pt1.X) / (pt2.Y - pt1.Y) + pt1.X);
        pt1.X = deltaX;
        pt1.Y = y;

    }
    else if(pt2.Y < y)
    {
        auto deltaX = int64_t(double(y - pt1.Y) * (pt2.X - pt1.X) / (pt2.Y - pt1.Y) + pt1.X);
        pt2.X = deltaX;
        pt2.Y = y;
    }
    if(pt1.X < pt2.X)
    {
        if(pt1.X < minX)
        {
            auto deltaY = int64_t(double(minX - pt1.X) * (pt2.Y - pt1.Y) / (pt2.X - pt1.X) + pt1.Y);
            pt1.X = minX;
            pt1.Y = deltaY;
        }
        if(pt2.X > maxX)
        {
            auto deltaY = int64_t(double(maxX - pt1.X) * (pt2.Y - pt1.Y) / (pt2.X - pt1.X) + pt1.Y);
            pt2.X = maxX;
            pt2.Y = deltaY;
        }
    }
    else
    {
        if(pt2.X < minX)
        {
            auto deltaY = int64_t(double(minX - pt1.X) * (pt2.Y - pt1.Y) / (pt2.X - pt1.X) + pt1.Y);
            pt2.X = minX;
            pt2.Y = deltaY;
        }
        if(pt1.X > maxX)
        {
            auto deltaY = int64_t(double(maxX - pt1.X) * (pt2.Y - pt1.Y) / (pt2.X - pt1.X) + pt1.Y);
            pt1.X = maxX;
            pt1.Y = deltaY;
        }
    }
    return (pt1.Y == y && pt2.Y == y);
}
