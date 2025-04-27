#ifndef SLASCANINFODEF_H
#define SLASCANINFODEF_H

#include <QList>
#include <QVector>
#include <QtGlobal>
#include <QSharedPointer>

enum INNERPERPARAMETERS {
    LIMITTYPE_SMALL2BIG = 0,
    LIMITTYPE_BIG2SMALL,

    KMODE_HORIZONTAL,
    KMODE_NORMAL,
    KMODE_VERTICAL,

    SORTTYPE_X,
    SORTTYPE_Y,

    LINETYPE_ORIGINAL,
    LINETYPE_SORTPATH_NEAR,
    LINETYPE_SORTPATH_TRIANGLE,
    LINETYPE_SORTPATH_GREED
};

struct EXPECTPOINTINFO {
    quint32 nPtPos;
    quint32 nAreaIndex;
    double X;
    double Y;
};
typedef QSharedPointer<EXPECTPOINTINFO> EXPECTPTINFOPTR;

typedef struct _LINECOOR {
    quint32 nPtPos_1;   // 起点在轮廓上的点索引
    quint32 nPtPos_2;   // 终点在轮廓上的点索引
    quint32 nArea_1;    // 起点所在轮廓的索引
    quint32 nArea_2;    // 终点所在轮廓的索引
    int X1;
    int Y1;
    int X2;
    int Y2;
}LINECOOR;

typedef QSharedPointer<LINECOOR> LINECOORPTR;

typedef struct __HATCHINGLINE {
    QVector<LINECOOR> listLineCoor;
    qint64 nLineIndex;
}HATCHINGLINE;
typedef QSharedPointer<HATCHINGLINE> HATCHINGLINEPTR;

typedef QVector<HATCHINGLINE> TOTALHATCHINGLINE;

struct REFLINEINFO {
    int nKMode = 0;
    int nSortType = 0;
    int nLimitType = 0;
    double X1 = 0.0;
    double Y1 = 0.0;
    double X2 = 0.0;
    double Y2 = 0.0;
    double fDelta_X = 0.0;
    double fDelta_Y = 0.0;
    double fLimit_X = 0.0;
    double fLimit_Y = 0.0;
};

#endif // SLASCANINFODEF_H
