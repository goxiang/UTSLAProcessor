#ifndef ALGORITHMAPPLICATION_H
#define ALGORITHMAPPLICATION_H

#include "./DynamicDivider/algorithmdistribution.h"
#include "algorithmhatching.h"
struct SOLIDPATH;

//namespace Utek {struct StartChangerPara;}
//using namespace Utek;
//struct Utek::StartChangerPara;

namespace Utek {
struct STARTCHANGERPARA;
}

struct CB_WRITEDATA {
    bool _getScanLines = false;
    std::function<void(const UFFWRITEDATA &, const int &, const int &)> _addWriteData =
        [](const UFFWRITEDATA &, const int &, const int &){ };
};
struct LatticeLayerInfo;
struct PartResult;
class ScanLinesSortor;
class SLMSplicingModule;
struct SortLineInfo;
struct MatchedLineInfo;
struct DivideSolidData;
typedef std::function<void(Paths &, Paths &, const CB_WRITEDATA &)> FunWriteHatching;
typedef std::function<void(const int64_t &, const int64_t &, const int64_t &,
                           const int64_t &, QVector<UFFWRITEDATA> &)> FuncSplitScanData;



class AlgorithmApplication : public AlgorithmHatching
{
public:
    void setBuffParas(PARAWRITEBUFF *, const int &curScanner = 0);
    inline void setCurLayerHei(const int &layerHei) { _layerHei = layerHei; }
    inline void setExportFileMode(const int &mode) { nExportFileMode = mode;}
    void calcSplicingVec();
    void createEnhanceBorderParas();
    void writeHatchingData(SOLIDPATH &, Paths &, const double &, const double &, const double &);
    void writeHatchingData(Paths &, const double &, const double &, const double &);
    void writeHatchingData(LatticeLayerInfo &, const double &, const double &, const double &);
    void setHatchingAngle(double);
    void addHatchingAngle();
    void addSolidSupportHatchingAngle();
    void writeSupportToSList(QList<AREAINFOPTR> &, const double &);
    void writeBorderData(const Paths &, const double &);
    void writeExternedBorder(Paths &, const double &);
    void keepHolesAndGaps(Paths *, Paths &, Paths &, Paths &);
    QMap<int, Paths> splitPathsByFiled(const Paths &);
    void writeSolidSupportData(Paths &, const double &borderSpeedFactor, const double &, const double &, const double &);

    void writeHoleAndGapData(Paths &, Paths &, const double &);
    void writeHoleAndGapData(Paths &, const int &);
    void writeHoleAndGapData(Paths &, const int &, const int &, const int &nScanner);

    bool createDistributionArea(const Paths &, const int &);
    void writeSupportAreaToPaths(QList<AREAINFOPTR> &, Paths &);
    void writeSupportToSList(const Paths &, const double &);

    void resetDistributionInfo();
    void setBorderDistributionInfo(const std::vector<PartResult> &);
    void setSupportDistributionInfo(const std::vector<PartResult> &);
    void setSolidDistributionInfo(const std::vector<PartResult> &);
    void setUpsurfaceDistributionInfo(const std::vector<PartResult> &);
    void setDownsurfaceDistributionInfo(const std::vector<PartResult> &);

    void calcDivideData(SOLIDPATH &, DivideSolidData &);
    void writeDivideData(QSharedPointer<DivideSolidData> &, const double &, const double &, const double &);

private:
    void getUpDownSurface(Paths &, Paths &, int &, Paths *, Paths *[], const int &);
    void writeAllHatching(const Paths &src, const int &beamIndex, const double &fSpeedRatio, const double &, FunWriteHatching &&,
                          const CB_WRITEDATA &func = CB_WRITEDATA(), const int &areaType = -1);
    void getBeamStruct(Paths &, Paths &, const int &BeamIndex);
    void recalcBeamStruct(const Paths &, const Paths &, Paths &, const double &);

    void writeHatching(const Paths &, const double &, const double &, const CB_WRITEDATA &cbFunc = CB_WRITEDATA());
    void writeDownFileData(Paths &, const int &, const double &, const double &);
    void writeUpFileData(Paths &, const int &, const double &, const double &);
    void writeSmallStruct(Paths &, const double &, const double &, const int &BeamIndex, const CB_WRITEDATA &, const int &areaType = -1);
    void writeBorderData(Paths &, const int &, const double &, const int &nScanner);
    void writeBorderData_splicing(Paths &, const int &, const double &, const int &nScanner);
    void writeExternedBorder_p(Paths &, const double &, const int &nScanner);
    void writeSolidSupportBorderData(const Paths &, const int &, const double &, const int &nScanner);

private:
    void writeHatching_p(const Paths &, const double &, const double &, const int &nScanner, const CB_WRITEDATA &);
    void writeDownFileData_p(const Paths &, const int &, const double &, const double &, const int &nScanner);
    void writeUpFileData_p(const Paths &, const int &, const double &, const double &, const int &nScanner);
    void writeSmallStruct_p(Paths &, const double &, const double &, const int &nScanner, const int &BeamIndex, const CB_WRITEDATA &);

    void writeSupportToSList_p(QList<AREAINFOPTR> *, const double &, const FuncSplitScanData &&);
    void writeSolidSupportHatching_p(const Paths &, const double &, const double &, const int &nScanner);

    void writeSupportToSList(UFFWRITEDATA &, const double &, const int &nScanner);
    void writeUPDownData(const Paths &, const int &, const double &, const int &, UPSURFACEPARAMETERS *,
                         UFFWRITEDATA &);
    int getUpDownMinWall(DOWNSURFACEPARAMETERS *);
    void splitLineByVerLine(const int64_t &, const int64_t &, const int64_t &, const int64_t &, QVector<UFFWRITEDATA> &);

    void writeAreaToPaths(QList<AREAINFOPTR> &, Paths &);
    void parsingDatas(QList<AREAINFOPTR> &, QVector<SCANLINE> &);
    void getNewBegin(Paths *lpPaths);

    void calcLineOnSplicing(Paths &);
    void calcLineOnSplicing(Paths &, SplicingLine &);

    void recalcSupportList(QVector<SCANLINE> &);
    void recalcSupportList(QVector<SortLineInfo> &, QList<int> &);
    int calcMinDisPos(const QVector<SortLineInfo> &, const QList<int> &, const int &, const int &, int &);

    bool isBigBeam(const int &BeamCnt, const int &BeamIndex);

    void writePathsToScanLines(const Path &, QVector<SCANLINE> &, const bool &closedPath = false);
    void writePathsToScanLines(const Paths &, QVector<SCANLINE> &, const bool &closedPath = false);

private:
    int _layerHei = 0;
    bool m_bPlatPlus = false;
    double fHatchingAngle = 0;
    double fSolidSupportHatchingAngle = 0;

    int curScannerIndex = 0;
    int nExportFileMode = 0;
    PARAWRITEBUFF *_writeBuff = nullptr;
    QSharedPointer<Utek::STARTCHANGERPARA> lpBEnhancedParas = nullptr;
//    QSharedPointer<SLMSplicingModule> splicingPtr = nullptr;
    QSharedPointer<ScanLinesSortor> scanLinesSortor = nullptr;
    QVector<SplicingArea> vecSplicingArea;
    QVector<SplicingLine> vecSplicingLine;

    std::vector<PartResult> _borderDisInfoVec;
    std::vector<PartResult> _supportDisInfoVec;
    std::vector<PartResult> _solidDisInfoVec;
    std::vector<PartResult> _upsurfaceDisInfoVec;
    std::vector<PartResult> _downsurfaceDisInfoVec;

    std::vector<Path> _distributionPathVec;
    std::vector<Path> _distributionUpsurfaceVec;
    std::vector<Path> _distributionDownsurfaceVec;
};

#endif // ALGORITHMAPPLICATION_H
