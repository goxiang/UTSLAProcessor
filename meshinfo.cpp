#include "meshinfo.h"

struct MPoint {
    int _x = 0;
    int _y = 0;

    MPoint(const int &x = 0, const int &y = 0) {
        _x = x;
        _y = y;
    }
};

inline bool operator==(const MPoint &pt1, const MPoint &pt2) {
    return (pt1._x == pt2._x) && (pt1._y == pt2._y);
}

MeshInfo *MeshInfo::getInstance()
{
    // auto funcGetCoorInDistance = [](const int &distance,
    //                                 const std::function<void(const int &, const int &)> &callFunc) {
    //     int radius = distance;
    //     int x = radius;
    //     int y = 0;
    //     int decision = 1 - x;

    //     QSet<MPoint> ptSet;

    //     auto funcInsetPt = [&ptSet, &callFunc](const int &x, const int &y){
    //         auto pt = MPoint(x, y);
    //         if (false == ptSet.contains(pt)) {
    //             ptSet << pt;
    //             callFunc(x,  y);
    //         }
    //     };

    //     while (x >= y) {
    //         funcInsetPt( x,  y);
    //         funcInsetPt(-x,  y);
    //         funcInsetPt( x, -y);
    //         funcInsetPt(-x, -y);
    //         funcInsetPt( y,  x);
    //         funcInsetPt(-y,  x);
    //         funcInsetPt( y, -x);
    //         funcInsetPt(-y, -x);

    //         y++;
    //         if (decision <= 0) {
    //             decision += 2 * y + 1;
    //         }
    //         else {
    //             x--;
    //             decision += 2 * (y - x) + 1;
    //         }
    //     }
    // };

    // int nCount = 0;
    // auto funcLogXY = [&nCount](const int &x, const int &y) {
    //     ++ nCount;
    // };
    // for (int i = 1; i < 100; ++ i) {
    //     nCount = 0;
    //     funcGetCoorInDistance(i, funcLogXY);
    //     qDebug() << i << "nCount"  << nCount << (2 * i + 1 + 2 * i - 1) * 2;
    // }

    static MeshInfo instance;
    return &instance;
}

const QVector<OffsetCoor> MeshInfo::getCoorList(const int &step)
{
    if (0 == step) return QVector<OffsetCoor>() << OffsetCoor(0, 0);

    QVector<OffsetCoor> listCoor;
    for(int i = - step; i <= step; ++ i)
    {
        listCoor << OffsetCoor(i, step);
        listCoor << OffsetCoor(i, - step);
    }
    for(int i = - step + 1; i <= step - 1; ++ i)
    {
        listCoor << OffsetCoor(step, i);
        listCoor << OffsetCoor(- step, i);
    }
    return listCoor;
    // return offsetList[step];
}
