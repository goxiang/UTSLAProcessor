#ifndef ALGORITHMBASE_H
#define ALGORITHMBASE_H

#include "bpccommon.h"
#include "publicheader.h"

class WriteUFF;

///
/// ! @coreclass{AlgorithmBase}
/// 提供基础的算法支持，包括几何计算、路径处理、扫描线生成等，独立类
class AlgorithmBase
{
public:
    double calcAreaList(const Paths &, QList<double> &);

    void calcLimitXY(const Paths &, BOUNDINGRECT &, QList<BDRECTPTR> *listRc = nullptr);
    void calcLimitXY(const QList<AREAINFOPTR> &, BOUNDINGRECT &, QList<BDRECTPTR> *listRc = nullptr);
    void getAllPaths(const QList<AREAINFOPTR> &, Paths &);


    bool lineCrossRC(const double &, const double &, const double &, const double &,
                     const BOUNDINGRECT &);
    bool lineCrossRC(const double &, const double &, const double &, const double &,
                     const BDRECTPTR &);

    void sortPtOnLine(int, QList<EXPECTPOINTINFO> &);
    void sortPtOnLine(int, QVector<EXPECTPOINTINFO> &);

    void reducePaths_Inner(Paths &);
    void addPaths_Upper(Paths &, Paths *);
    void extendBorders(Paths *, Paths *, Paths *);

    void getSurfacePaths(Paths &, int &, Paths *, Paths *[], const int &, const int &);
    void getDifferencePaths(const Paths &, const Path &, Paths &, const int &);
    void getDifferencePaths(const Paths *, const Paths *, Paths &, const int &);
    void getUnionPaths(const Path &, const Path &, Paths &, const int &);
    void getUnionPaths(const Path &, const Paths &, Paths &, const int &);
    void getUnionPaths(const Paths *, const Paths *, Paths &, const int &);
    void getOffsetPaths(Paths *, int nOffset, int nReserved, int nSmallWall);
    void getOffsetPaths(Paths *, Paths *, const double &, const double &, const double &);

    void getIntersectionPaths(const Paths &, const Path &, Paths &);
    void getIntersectionPaths(const Paths &, const Paths &, Paths &);


    bool lineOnVecSplicingLine(int64_t, int64_t, int64_t, int64_t, QVector<SplicingLine> &);
    void filteSmallBorder(const Paths &, Paths &, Paths &, const double &);

    static void scanner_jumpPos(const int &, const int &, QVector<SCANLINE> &);
    static void scanner_outLine(const int &, const int &, QVector<SCANLINE> &);
    static void scanner_jumpPos(const qint64 &, const qint64 &, QVector<SCANLINE> &);
    static void scanner_outLine(const qint64 &, const qint64 &, QVector<SCANLINE> &);
    static void scanner_plotLine(const int &, const int &, const int &, const int &, QVector<SCANLINE> &);
    static void writeDataToList(const int64_t &, const int64_t &, const int64_t &, const int64_t &,
                                int64_t &, int64_t &, QVector<SCANLINE> &);

private:
    void calcLimitXY(const Path &, BDRECTPTR &);
    void calcLimitXY(const AREAINFOPTR &, BDRECTPTR &);
};

#endif // ALGORITHMBASE_H
