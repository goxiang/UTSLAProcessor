#ifndef ALGORITHMHATCHING_H
#define ALGORITHMHATCHING_H

#include "algorithmstrip.h"
#include "algorithmchecker.h"
#include "algorithmhatchingring.h"
#include "UTSLAProcessor_global.h"

struct PocketBranch;
///
/// ! @coreclass{AlgorithmHatching}
/// 填充算法基类,继承自
/// 1.AlgorithmHatchingRing
/// 负责环形填充算法
/// 处理圆形或环形结构的填充路径
/// 2.AlgorithmChecker
/// 负责检查和验证算法
/// 确保生成路径的正确性和有效性
/// 3.AlgorithmStrip
/// 负责条带式填充算法
/// 处理直线型填充路径
///
class AlgorithmHatching : public AlgorithmHatchingRing, public AlgorithmChecker,
        public AlgorithmStrip
{
public:
    void calcInnerPoint(const Paths &, TOTALHATCHINGLINE &, int &nTotalLCnt, const double &,
                        const int &, const bool &bInner = false);
    void calcRingHatching(const Paths &, QVector<SCANLINE> &, const int &nSpacing);
    void calcSpiralHatching(const Paths &, QVector<SCANLINE> &, const int &nSpacing);
    void calcFermatSpiralHatching(const Paths &, QVector<SCANLINE> &, const int &nSpacing);

    void calcRingTree(const Paths &, PocketTree &, const int &nSpacing);

    void calcSPLine(const int &, TOTALHATCHINGLINE &, QVector<SCANLINE> &,
                    const int &nTLineCnt = 0, const int &nLSConnecter = 0, const int &nCLineFactor = 0);

    void drawBranch(const PocketBranch &branch, QVector<SCANLINE> &);
    void drawBranch_spiral(const PocketBranch &branch, QVector<SCANLINE> &);
    void drawBranch_fermatSpiral(const PocketBranch &branch, QVector<SCANLINE> &);
#ifdef USE_PROCESSOR_EXTEND
    void drawPathOffset(const Path &, const IntPoint &, const ulong &ms = 0);
    void drawPathOffset(const Paths &, const IntPoint &, const ulong &ms = 0);
#endif
    void drawPathToScanLine(const Paths &, QVector<SCANLINE> &);
    void drawPathToScanLine(const Path &, QVector<SCANLINE> &, const int &type, const IntPoint &offsetCoor = IntPoint(0, 0));

    void drawSpiralPathToScanLine(const Path &, QVector<SCANLINE> &);
    bool getFirstLine(const int &, const int &, int *, int *, TOTALHATCHINGLINE &, bool &bFirstLine);
    bool getNextLine(const int &, const int &, int *, int *, TOTALHATCHINGLINE &, const int &nLConneter);
    bool getFirstLine_Greed(const int &, const int &, int *, int *, TOTALHATCHINGLINE &, bool &bFirstLine);
};

#endif // ALGORITHMHATCHING_H
