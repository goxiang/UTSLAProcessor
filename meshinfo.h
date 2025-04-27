#ifndef MESHINFO_H
#define MESHINFO_H

#include <QList>
#include <QVector>
#include <QDebug>

#define glMeshInfo MeshInfo::getInstance()

struct SortLineInfo {
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    int nStartPos = 0;
    int nEndPos = 0;
    bool reversed = false;
};

struct MatchedLineInfo {
    int endPtX = 0;
    int endPtY = 0;
    int meshCoorX = 0;
    int meshCoorY = 0;

    int lineIndex = -1;
    bool needReverse = false;
    double delta = 1e300;

    void resetValue(SortLineInfo *lpLine) {
        if(false == lpLine->reversed)
        {
            endPtX = lpLine->x2;
            endPtY = lpLine->y2;
        }
        else
        {
            endPtX = lpLine->x1;
            endPtY = lpLine->y1;
        }
        lineIndex = -1;
        delta = 1e300;
        needReverse = false;
    }
};

struct OffsetCoor {
    int nOffsetX = 0;
    int nOffsetY = 0;

    OffsetCoor(const int &x = 0, const int &y = 0) {
        nOffsetX = x;
        nOffsetY = y;
    }

    bool operator==(const OffsetCoor &coor) {
        return (nOffsetX == coor.nOffsetX && nOffsetY == coor.nOffsetY);
    }
};

class MeshInfo
{
public:
    static MeshInfo *getInstance();
    const QVector<OffsetCoor> getCoorList(const int &);

private:
    MeshInfo() = default;

};

inline QDebug operator<<(QDebug debug, const OffsetCoor &p)
{
    debug << "(" << p.nOffsetX << "," << p.nOffsetY << ") ";
    return debug;
}

#endif // MESHINFO_H
