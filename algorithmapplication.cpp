#include "algorithmapplication.h"
#include "polygonstartchanger.h"

#include "ScanLinesDivider/scanlinesdivider.h"
#include "PolygonsDivider/polygonsdivider.h"
#include "ScanLinesSortor/scanlinessortor.h"
#include "LatticeModule/latticeinterface.h"
#include "slmsplicingmodule.h"
#include "clipper2/clipper.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>

#define CALC_HATCHING
#define CALC_SMALLSTRUCT
#define CALC_SUPPORT
#define CALC_BORDER

#ifdef CALC_BORDER
#define CALC_SMALLGAP_SMALLHOLE
#define CALC_EXTERNEDBORDER
#endif

#define USE_CHECKER_BOARD_TEST

enum {
    HATCHTYPE_LINES         = 0,
    HATCHTYPE_RING          = 1,
    HATCHTYPE_SPIRAL        = 2,
    HATCHTYPE_FERMATSPIRAL  = 3,
    HATCHTYPE_CHECKER       = 4,
    HATCHTYPE_STRIP         = 5
};

void AlgorithmApplication::setBuffParas(PARAWRITEBUFF *paras, const int &curScanner)
{
    _writeBuff = paras;
    curScannerIndex = curScanner;
    scanLinesSortor = QSharedPointer<ScanLinesSortor>
        (new ScanLinesSortor(BpcParas->nPlatWidthX, BpcParas->nPlatWidthY));
#ifndef USE_CHECKER_BOARD_TEST
    if(buffParas->mExtendObj.contains("CheckerParas"))
    {
        auto jsonObj = buffParas->mExtendObj["CheckerParas"].toObject();
        createCheckers(jsonObj,
                       BppParas->sPlatformPara.nPlatWidthX * UNITSPRECISION,
                       BppParas->sPlatformPara.nPlatWidthY * UNITSPRECISION);
    }
#else
    {
        QJsonObject jsonObj;
        jsonObj["fWid"] = ExtendedParas<double>("CheckBorder/fWid", 10);
        jsonObj["fHei"] = ExtendedParas<double>("CheckBorder/fHei", 10);
        jsonObj["fRotateAngle"] = ExtendedParas<double>("CheckBorder/fRotateAngle", 45.0);
        jsonObj["fOverlapWid"] = ExtendedParas<double>("CheckBorder/fOverlapWid", 1);
        jsonObj["fOffsetChecker"] = ExtendedParas<double>("CheckBorder/fOffsetChecker", 1);
        jsonObj["nScanCheckerBorder"] = ExtendedParas<int>("CheckBorder/nScanCheckerBorder", 0);
        createCheckers(jsonObj, BpcParas->nPlatWidthX * UNITSPRECISION,
                       BpcParas->nPlatWidthY * UNITSPRECISION);
    }
    {
        QJsonObject jsonObj;
        jsonObj["fStripHei"] = ExtendedParas<double>("Strip/fStripHei", 10);
        jsonObj["fSpacing"] = ExtendedParas<double>("Strip/fSpacing", 0.13);
        createStrips(jsonObj, BpcParas->nPlatWidthX * UNITSPRECISION,
                     BpcParas->nPlatWidthY * UNITSPRECISION);
    }
#endif
}

void AlgorithmApplication::setHatchingAngle(double fAngle)
{
    fHatchingAngle = fAngle;
}

void AlgorithmApplication::addHatchingAngle()
{
    int nIterationCnt = 0;
LABEL_CALC_HATCHING_ANGLE:
    fHatchingAngle += double(BppParas->sHatchingPara.fAngle);
    fHatchingAngle = ceil(fHatchingAngle * 100) / 100;
    while(fHatchingAngle > 360) fHatchingAngle -= 360;

    if(EXPORT_SLM_BUILDFILE == nExportFileMode)
    {
        auto limitAngleParas = BpcParas->mExtendParasObj["LimitAngleParas"].toObject();

        auto jsonArr = limitAngleParas["LimitAngles"].toArray();
        if(jsonArr.size())
        {
            auto tempAngle = fHatchingAngle + 90;
            while(tempAngle > 360) tempAngle -= 360;

            bool bRecalculate = false;
            for(const auto &obj : qAsConst(jsonArr))
            {
                auto tempObj = obj.toObject();
                if((tempAngle > tempObj["nMinLimit"].toInt() && tempAngle < tempObj["nMaxLimit"].toInt())
                    || (fabs(tempAngle - tempObj["nMinLimit"].toInt()) < 1E-6)
                    || (fabs(tempAngle - tempObj["nMaxLimit"].toInt()) < 1E-6))
                {
                    if(nIterationCnt ++ < limitAngleParas["nMaxIteration"].toInt()) bRecalculate = true;
                    break;
                }
            }
            if(bRecalculate) goto LABEL_CALC_HATCHING_ANGLE;
        }
    }
    changeLayerOddInfo();
}

void AlgorithmApplication::addSolidSupportHatchingAngle()
{
    int nIterationCnt = 0;
LABEL_CALC_HATCHING_ANGLE:
    fSolidSupportHatchingAngle += ExtendedParas<double>("SolidSupportHatching/fAngle", 60.3);
    fSolidSupportHatchingAngle = ceil(fSolidSupportHatchingAngle * 100) / 100;
    while(fSolidSupportHatchingAngle > 360) fSolidSupportHatchingAngle -= 360;

    if(EXPORT_SLM_BUILDFILE == nExportFileMode)
    {
        auto limitAngleParas = BpcParas->mExtendParasObj["LimitAngleParas"].toObject();

        auto jsonArr = limitAngleParas["LimitAngles"].toArray();
        if(jsonArr.size())
        {
            auto tempAngle = fSolidSupportHatchingAngle + 90;
            while(tempAngle > 360) tempAngle -= 360;

            bool bRecalculate = false;
            for(const auto &obj : qAsConst(jsonArr))
            {
                auto tempObj = obj.toObject();
                if((tempAngle > tempObj["nMinLimit"].toInt() && tempAngle < tempObj["nMaxLimit"].toInt())
                    || (fabs(tempAngle - tempObj["nMinLimit"].toInt()) < 1E-6)
                    || (fabs(tempAngle - tempObj["nMaxLimit"].toInt()) < 1E-6))
                {
                    if(nIterationCnt ++ < limitAngleParas["nMaxIteration"].toInt()) bRecalculate = true;
                    break;
                }
            }
            if(bRecalculate) goto LABEL_CALC_HATCHING_ANGLE;
        }
    }
}

void AlgorithmApplication::calcSplicingVec()
{
    vecSplicingArea.clear();
    vecSplicingLine.clear();

    BPCUSTOMERPARAS *lpParas = BpcParas.data();
    if(lpParas->nNumber_SplicingScanner < 2) return;

    int nPlatY = lpParas->nPlatWidthY * FILEDATAUNIT;
    lpParas->fSplicingPos[lpParas->nNumber_SplicingScanner - 1] = lpParas->nPlatWidthX + 100;
    lpParas->fSplicingLength[lpParas->nNumber_SplicingScanner - 1] = 0;
    int nLeftPos = 0;
    int nRightPos = 0;
    for(int iScanner = 0; iScanner < lpParas->nNumber_SplicingScanner; ++ iScanner)
    {
        SplicingArea splitArea;
        int nHalfSplicingLength = qRound(double(lpParas->fSplicingLength[iScanner]) * FILEDATAUNIT_0_5);
        nRightPos = m_bPlatPlus ? qRound((lpParas->fSplicingPos[iScanner] * FILEDATAUNIT + nHalfSplicingLength))
                                : qRound((lpParas->fSplicingPos[iScanner] * FILEDATAUNIT - nHalfSplicingLength));
        splitArea.nSplicingPosLeft = nLeftPos;
        splitArea.nSplicingPosRight = nRightPos;
        splitArea.pathArea << IntPoint(nLeftPos, 0) << IntPoint(nRightPos, 0)
                           << IntPoint(nRightPos, nPlatY) << IntPoint(nLeftPos, nPlatY);
        vecSplicingArea << splitArea;

        if(iScanner)
        {
            SplicingLine splicingLine;
            splicingLine.nSplicingPos = nLeftPos;
            vecSplicingLine << splicingLine;
        }
        nLeftPos = nRightPos;
    }
    m_bPlatPlus = !m_bPlatPlus;
}

void AlgorithmApplication::createEnhanceBorderParas()
{
    ENHANCEDBORDER *lpEnhanceBorder = &BppParas->sEnhancedBorder;

    lpBEnhancedParas = QSharedPointer<Utek::STARTCHANGERPARA>(new Utek::STARTCHANGERPARA());
    lpBEnhancedParas->changerType = lpEnhanceBorder->nEnhanceMode;
    lpBEnhancedParas->insertAngle = float(tan((90.0 - double(lpEnhanceBorder->fSwingAngle)) * DEF_PI / 180.0));
    lpBEnhancedParas->insertLength = lpEnhanceBorder->fExtensionLength * FILEDATAUNIT;
    lpBEnhancedParas->smallPolyArea = lpEnhanceBorder->fMinArea * FILEDATAUNIT * FILEDATAUNIT;
    lpBEnhancedParas->smallPolyLength = lpEnhanceBorder->fMinEdgeLength * FILEDATAUNIT;
}

void AlgorithmApplication::writeHatchingData(SOLIDPATH &solidPath, Paths &extendPaths, const double &fSpeedRatio,
                                             const double &fLLineSpace, const double &fSLineSpace)
{
#ifdef CALC_HATCHING
    BPPBUILDPARAMETERS *lpBuildPara = BppParas.data();

#ifdef USE_PROCESSOR_EXTEND
    BppParas->sHatchingPara.fMinWall[1] = 0.8f;
    lpBuildPara->sSurfacePara_Up.nNumber = 0;
    lpBuildPara->sSurfacePara_Dw.nNumber = 0;
//    BppParas->sHatchingPara.nIndex_BeamSize[0] = 2;
//    BppParas->sHatchingPara.nIndex_BeamSize[1] = 1;
//    BppParas->sHatchingPara.nIndex_BeamSize[2] = 0;
#endif

    Paths curPath;
    curPath.resize(solidPath.lpPath_Cur->size());
    CleanPolygons(*solidPath.lpPath_Cur, curPath, MINDISTANCE);

    Paths paths_Inner;
    Paths paths_down, paths_up;
    int nIndex_down = -1;
    int nIndex_up = -1;

    if(0 == lpBuildPara->sSurfacePara_Up.nNumber && 0 == lpBuildPara->sSurfacePara_Dw.nNumber)
    {
        paths_Inner = curPath;
    }
    else
    {
        Paths paths_InnerLast;
        if(lpBuildPara->sSurfacePara_Dw.nNumber)
        {
            getUpDownSurface(paths_down, extendPaths, nIndex_down, &curPath, solidPath.lpPath_Dw,
                             lpBuildPara->sSurfacePara_Dw.nNumber);
        }
        if(lpBuildPara->sSurfacePara_Up.nNumber)
        {
            getUpDownSurface(paths_up, extendPaths, nIndex_up, &curPath, solidPath.lpPath_Up,
                             lpBuildPara->sSurfacePara_Up.nNumber);
        }
        if(lpBuildPara->sSurfacePara_Dw.nNumber)
        {
            getDifferencePaths(&curPath, &paths_down, paths_InnerLast, int(FILEDATAUNIT_0_05));
        }
        else
        {
            paths_InnerLast = curPath;
        }
        if(lpBuildPara->sSurfacePara_Up.nNumber)
        {
            getDifferencePaths(&paths_InnerLast, &paths_up, paths_Inner, int(FILEDATAUNIT_0_05));
        }
        else
        {
            paths_Inner = paths_InnerLast;
        }
    }

    double fLineSpace = isBigBeam(BppParas->sGeneralPara.nNumber_Beam, BppParas->sGeneralPara.nNumber_Beam - 1) ?
                            fLLineSpace : fSLineSpace;
    writeAllHatching(paths_Inner, HATCHINGBEAM_NORMAL, fSpeedRatio, fSLineSpace,
                     [paths_Inner, fSpeedRatio, fLineSpace, this](Paths &curPaths, Paths &lastPaths, const CB_WRITEDATA &) {
                         HATCHINGPARAMETERS *lpPara = &BppParas->sHatchingPara;
                         double fOffset = double(lpPara->fOffset[HATCHINGBEAM_NORMAL]) * FILEDATAUNIT * BpcParas->fFactor_8;
                         recalcBeamStruct(paths_Inner, lastPaths, curPaths, fOffset);
                         writeHatching(curPaths, fSpeedRatio, fLineSpace);
                     });
    if(paths_down.size())
    {
        auto lpParas = &BppParas->sSurfacePara_Dw;
        int nHatchingBeamIndex = BppParas->sGeneralPara.nNumber_Beam - 1 - lpParas->nIndex_BeamSize;
        fLineSpace = isBigBeam(BppParas->sGeneralPara.nNumber_Beam, lpParas->nIndex_BeamSize) ?
                         fLLineSpace : fSLineSpace;
        writeAllHatching(paths_down, nHatchingBeamIndex, fSpeedRatio, fSLineSpace,
                         [paths_down, fSpeedRatio, fLineSpace, nIndex_down, lpParas, this](Paths &curPaths, Paths &lastPaths, const CB_WRITEDATA &) {
                             double fOffset = double(lpParas->fOffset[nIndex_down] * FILEDATAUNIT);
                             recalcBeamStruct(paths_down, lastPaths, curPaths, fOffset);
                             writeDownFileData(curPaths, nIndex_down, fSpeedRatio, fLineSpace);
                         });
    }
    if(paths_up.size())
    {
        auto lpParas = &BppParas->sSurfacePara_Up;
        int nHatchingBeamIndex = BppParas->sGeneralPara.nNumber_Beam - 1 - lpParas->nIndex_BeamSize;
        fLineSpace = isBigBeam(BppParas->sGeneralPara.nNumber_Beam, lpParas->nIndex_BeamSize) ?
                         fLLineSpace : fSLineSpace;
        writeAllHatching(paths_up, nHatchingBeamIndex, fSpeedRatio, fSLineSpace,
                         [paths_up, fSpeedRatio, fLineSpace, nIndex_up, lpParas, this](Paths &curPaths, Paths &lastPaths, const CB_WRITEDATA &) {
                             double fOffset = double(lpParas->fOffset[nIndex_up] * FILEDATAUNIT);
                             recalcBeamStruct(paths_up, lastPaths, curPaths, fOffset);
                             writeUpFileData(curPaths, nIndex_up, fSpeedRatio, fLineSpace);
                         });
    }

#endif
}

void AlgorithmApplication::calcDivideData(SOLIDPATH &solidPath, DivideSolidData &divideData)
{
    BPPBUILDPARAMETERS *lpBuildPara = BppParas.data();
    Paths curPath;
    curPath.resize(solidPath.lpPath_Cur->size());
    CleanPolygons(*solidPath.lpPath_Cur, curPath, MINDISTANCE);

    if(0 == lpBuildPara->sSurfacePara_Up.nNumber && 0 == lpBuildPara->sSurfacePara_Dw.nNumber)
    {
        divideData._paths_Inner = curPath;
    }
    else
    {
        Paths paths_InnerLast;
        if(lpBuildPara->sSurfacePara_Dw.nNumber)
        {
            getUpDownSurface(divideData._paths_down, divideData._extendPaths, divideData._nIndex_down, &curPath, solidPath.lpPath_Dw,
                             lpBuildPara->sSurfacePara_Dw.nNumber);
        }
        if(lpBuildPara->sSurfacePara_Up.nNumber)
        {
            getUpDownSurface(divideData._paths_up, divideData._extendPaths, divideData._nIndex_up, &curPath, solidPath.lpPath_Up,
                             lpBuildPara->sSurfacePara_Up.nNumber);
        }
        if(lpBuildPara->sSurfacePara_Dw.nNumber)
        {
            getDifferencePaths(&curPath, &divideData._paths_down, paths_InnerLast, int(FILEDATAUNIT_0_05));
        }
        else
        {
            paths_InnerLast = curPath;
        }
        if(lpBuildPara->sSurfacePara_Up.nNumber)
        {
            getDifferencePaths(&paths_InnerLast, & divideData._paths_up, divideData._paths_Inner, int(FILEDATAUNIT_0_05));
        }
        else
        {
            divideData._paths_Inner = paths_InnerLast;
        }
    }
}

void AlgorithmApplication::writeDivideData(QSharedPointer<DivideSolidData> &divideData, const double &fSpeedRatio,
                                           const double &fLLineSpace, const double &fSLineSpace)
{
    double fLineSpace = isBigBeam(BppParas->sGeneralPara.nNumber_Beam, BppParas->sGeneralPara.nNumber_Beam - 1) ?
                            fLLineSpace : fSLineSpace;
    if (divideData->_paths_Inner.size())
    {
        writeAllHatching(divideData->_paths_Inner, HATCHINGBEAM_NORMAL, fSpeedRatio, fSLineSpace,
                         [&, this](Paths &curPaths, Paths &lastPaths, const CB_WRITEDATA &) {
                             HATCHINGPARAMETERS *lpPara = &BppParas->sHatchingPara;
                             double fOffset = double(lpPara->fOffset[HATCHINGBEAM_NORMAL]) * FILEDATAUNIT * BpcParas->fFactor_8;
                             recalcBeamStruct(divideData->_paths_Inner, lastPaths, curPaths, fOffset);
                             writeHatching(curPaths, fSpeedRatio, fLineSpace);
                         }, CB_WRITEDATA(), DistAreaType::Hatching);
    }
    if(divideData->_paths_down.size())
    {
        auto lpParas = &BppParas->sSurfacePara_Dw;
        int nHatchingBeamIndex = BppParas->sGeneralPara.nNumber_Beam - 1 - lpParas->nIndex_BeamSize;
        fLineSpace = isBigBeam(BppParas->sGeneralPara.nNumber_Beam, lpParas->nIndex_BeamSize) ?
                         fLLineSpace : fSLineSpace;
        writeAllHatching(divideData->_paths_down, nHatchingBeamIndex, fSpeedRatio, fSLineSpace,
                         [&, this](Paths &curPaths, Paths &lastPaths, const CB_WRITEDATA &) {
                             double fOffset = double(lpParas->fOffset[divideData->_nIndex_down] * FILEDATAUNIT);
                             recalcBeamStruct(divideData->_paths_down, lastPaths, curPaths, fOffset);
                             writeDownFileData(curPaths, divideData->_nIndex_down, fSpeedRatio, fLineSpace);
                         }, CB_WRITEDATA(), DistAreaType::Downface);
    }
    if(divideData->_paths_up.size())
    {
        auto lpParas = &BppParas->sSurfacePara_Up;
        int nHatchingBeamIndex = BppParas->sGeneralPara.nNumber_Beam - 1 - lpParas->nIndex_BeamSize;
        fLineSpace = isBigBeam(BppParas->sGeneralPara.nNumber_Beam, lpParas->nIndex_BeamSize) ?
                         fLLineSpace : fSLineSpace;
        writeAllHatching(divideData->_paths_up, nHatchingBeamIndex, fSpeedRatio, fSLineSpace,
                         [&, this](Paths &curPaths, Paths &lastPaths, const CB_WRITEDATA &) {
                             double fOffset = double(lpParas->fOffset[divideData->_nIndex_up] * FILEDATAUNIT);
                             recalcBeamStruct(divideData->_paths_up, lastPaths, curPaths, fOffset);
                             writeUpFileData(curPaths, divideData->_nIndex_up, fSpeedRatio, fLineSpace);
                         }, CB_WRITEDATA(), DistAreaType::Upface);
    }
}

void AlgorithmApplication::writeHatchingData(Paths &lattices, const double &fSpeedRatio,
                                             const double &fLLineSpace, const double &fSLineSpace)
{
    if (lattices.size() < 1) return;
    double fLineSpace = isBigBeam(BppParas->sGeneralPara.nNumber_Beam, BppParas->sGeneralPara.nNumber_Beam - 1) ?
                            fLLineSpace : fSLineSpace;
    writeAllHatching(lattices, HATCHINGBEAM_NORMAL, fSpeedRatio, fSLineSpace,
                     [lattices, fSpeedRatio, fLineSpace, this](Paths &curPaths, Paths &lastPaths, const CB_WRITEDATA &cbWriteData) {
                         HATCHINGPARAMETERS *lpPara = &BppParas->sHatchingPara;
                         double fOffset = double(lpPara->fOffset[HATCHINGBEAM_NORMAL]) * FILEDATAUNIT * BpcParas->fFactor_8;
                         recalcBeamStruct(lattices, lastPaths, curPaths, fOffset);
                         writeHatching(curPaths, fSpeedRatio, fLineSpace);
                     });
}

void AlgorithmApplication::writeHatchingData(LatticeLayerInfo &layerInfo, const double &fSpeedRatio,
                                             const double &fLLineSpace, const double &fSLineSpace)
{
    if (layerInfo._latticeInfo.size() < 1 || layerInfo._latticePathInfo.size() < 1) return;

    struct WriteDatas {
        UFFWRITEDATA _writeData;
        int _scanner;
        int _beam;

        WriteDatas() = default;
        WriteDatas(const UFFWRITEDATA &writeData, const int &scanner, const int &beam) {
            _writeData = std::move(writeData);
            _scanner = scanner;
            _beam = beam;
        }
    };

    auto keys = layerInfo._latticePathInfo.keys();
    QMap<int, QVector<WriteDatas>> writeDataMap;

    double fLineSpace = isBigBeam(BppParas->sGeneralPara.nNumber_Beam, BppParas->sGeneralPara.nNumber_Beam - 1) ?
                            fLLineSpace : fSLineSpace;

    int curType = 0;
    CB_WRITEDATA cb_writeData;
    cb_writeData._getScanLines = true;
    cb_writeData._addWriteData = [&curType, &writeDataMap](const UFFWRITEDATA &writeData, const int &scanner, const int &beam) {
        if (false == writeDataMap.contains(curType)) writeDataMap.insert(curType, QVector<WriteDatas>());
        writeDataMap[curType] << WriteDatas(writeData, scanner, beam);
    };

    for (const auto &key : qAsConst(keys))
    {
        curType = key;
        Paths tempPaths;
        tempPaths << layerInfo._latticePathInfo[key]._path;
        writeAllHatching(tempPaths, HATCHINGBEAM_NORMAL, fSpeedRatio, fSLineSpace,
                         [&tempPaths, fSpeedRatio, fLineSpace, this](Paths &curPaths, Paths &lastPaths, const CB_WRITEDATA &cbWriteData) {
                             HATCHINGPARAMETERS *lpPara = &BppParas->sHatchingPara;
                             double fOffset = double(lpPara->fOffset[HATCHINGBEAM_NORMAL]) * FILEDATAUNIT * BpcParas->fFactor_8;
                             recalcBeamStruct(tempPaths, lastPaths, curPaths, fOffset);
                             writeHatching(curPaths, fSpeedRatio, fLineSpace, cbWriteData);
                         }, cb_writeData);
    }

    for (const auto &lattice : qAsConst(layerInfo._latticeInfo))
    {
        auto type = lattice._type;
        auto writeDataVec = writeDataMap[type];
        auto vecSz = writeDataVec.size();
        if (vecSz < 1) continue;

        auto offsetX = lattice._centerX - layerInfo._latticePathInfo[type]._centerX;
        auto offsetY = lattice._centerY - layerInfo._latticePathInfo[type]._centerY;

        for (int i = 0; i < vecSz; ++ i)
        {
            auto writeData = writeDataVec[i];
            if (offsetX || offsetY)
            {
                for (auto it = writeData._writeData.listSLines.begin();
                     it != writeData._writeData.listSLines.end(); ++ it)
                {
                    it->nX += offsetX;
                    it->nY += offsetY;
                }
            }
            _writeBuff->appendFileData(writeData._writeData, writeData._scanner, writeData._beam);
        }
    }
}

void AlgorithmApplication::getUpDownSurface(Paths &targetPath, Paths &extendPaths, int &nIndex,
                                            Paths *lpCurPath, Paths *lpPaths[], const int &number)
{
    getSurfacePaths(targetPath, nIndex, lpCurPath, lpPaths, int(FILEDATAUNIT_0_2), number);
    if(-1 != nIndex)
    {
        reducePaths_Inner(targetPath);
        extendBorders(&targetPath, lpCurPath, &extendPaths);
        addPaths_Upper(targetPath, lpCurPath);
        SimplifyPolygons(targetPath, pftEvenOdd);
    }
}

void AlgorithmApplication::writeAllHatching(const Paths &src, const int &beamIndex, const double &fSpeedRatio,
                                            const double &fSpacingRatio, FunWriteHatching &&funHatching, const CB_WRITEDATA &func,
                                            const int &areaType)
{
    int nBeamIndexCnt = BppParas->sGeneralPara.nNumber_Beam - 1;
    Paths paths = src;
    Paths paths_Small = src;
    QMap<int, Paths> beamStructMap;

    for(int iBeam = nBeamIndexCnt; iBeam >= beamIndex; -- iBeam)
    {
        if(paths.size() < 1) continue;
        if(beamIndex == iBeam)
        {
            if(paths.size()) beamStructMap[iBeam] = paths;
        }
        else
        {
            getBeamStruct(paths, paths_Small, iBeam);
            if(paths_Small.size()) beamStructMap[iBeam] = paths_Small;
        }
    }

    auto keys = beamStructMap.keys();
    std::sort(keys.begin(), keys.end(),
              [](const int &x1, const int &x2) -> bool { return x1 > x2; });
    Paths lastPaths;
    for(const auto key : qAsConst(keys))
    {
        auto &curPaths = beamStructMap[key];
        if(0 == key || key == beamIndex) funHatching(curPaths, lastPaths, func);
        else
        {
            HATCHINGPARAMETERS *lpPara = &BppParas->sHatchingPara;
            double fOffset = double(lpPara->fOffset[key]) * FILEDATAUNIT * BpcParas->fFactor_7;
            recalcBeamStruct(src, lastPaths, curPaths, fOffset);
            lastPaths = curPaths;
            writeSmallStruct(curPaths, fSpeedRatio, fSpacingRatio, key, func, areaType);
        }
    }
}

void AlgorithmApplication::getBeamStruct(Paths &src, Paths &paths_Small, const int &BeamIndex)
{
    if(src.size() < 1) return;

    Paths paths_Big;
    HATCHINGPARAMETERS *lpPara = &BppParas->sHatchingPara;
    getOffsetPaths(&src, &paths_Big, 0,
                   qRound(double(lpPara->fMinWall[BeamIndex - 1]) * FILEDATAUNIT_0_5), FILEDATAUNIT_0_05);
    getDifferencePaths(&src, &paths_Big, paths_Small, int(FILEDATAUNIT_0_05));
    if(paths_Small.size())
    {
        getDifferencePaths(&src, &paths_Small, src, int(FILEDATAUNIT_0_05));
    }
}

void AlgorithmApplication::recalcBeamStruct(const Paths &src, const Paths &paths_Last, Paths &paths_Small, const double &fOffset)
{
    if(paths_Small.size() < 1) return;
    getOffsetPaths(&paths_Small, - qRound(fOffset * 0.8), 0, 0);
    getUnionPaths(&src, &paths_Small, paths_Small, int(FILEDATAUNIT_0_05));
    getOffsetPaths(&paths_Small, qRound(fOffset), 0, 0);

    getDifferencePaths(&paths_Small, &paths_Last, paths_Small, 0);
}

void AlgorithmApplication::writeHatching(const Paths &paths, const double &fSpeedRatio, const double &fSpacingRatio, const CB_WRITEDATA &cbFunc)
{
    if(paths.size() < 1) return;
    const auto &pathInner_Temp = paths;

    if(_writeBuff->isSolidSplicing())
    {
        if(EXPORT_SLM_BUILDFILE == nExportFileMode)
        {
            SplicingPtr->splitSolidData(SplicingPtr->getLayerSplicing(_layerHei), pathInner_Temp,
                                        [=](const int &index, const Paths &paths) {
                                            writeHatching_p(paths, fSpeedRatio, fSpacingRatio, index, cbFunc);
                                        });
        }
        else
        {
            Paths pathTarget;
            int nScanner = vecSplicingArea.size();
            for(int iScanner = 0; iScanner < nScanner; ++ iScanner)
            {
                getIntersectionPaths(pathInner_Temp, vecSplicingArea.at(iScanner).pathArea, pathTarget);
                writeHatching_p(pathTarget, fSpeedRatio, fSpacingRatio, iScanner, cbFunc);
            }
        }
    }
    else
    {
        if (_solidDisInfoVec.size() > 1)
        {
            long long index = 0;
            for (const auto &path : qAsConst(_distributionPathVec))
            {
                Paths tempPaths;
                getIntersectionPaths(pathInner_Temp, path, tempPaths);
                writeHatching_p(tempPaths, fSpeedRatio, fSpacingRatio, _solidDisInfoVec[index]._containorIndex, cbFunc);
                ++ index;
            }
        }
        else
        {
            auto tempIndex = curScannerIndex;
            if (1 == _solidDisInfoVec.size()) tempIndex = _solidDisInfoVec.front()._containorIndex;
            writeHatching_p(pathInner_Temp, fSpeedRatio, fSpacingRatio, tempIndex, cbFunc);
        }
    }
}

void AlgorithmApplication::writeDownFileData(Paths &paths_Down, const int &nSurfaceIndex, const double &fSpeedRatio,
                                             const double &fSpacingRatio)
{
    DOWNSURFACEPARAMETERS *lpParas = &BppParas->sSurfacePara_Dw;
    if(lpParas->nNumber < 1 || paths_Down.size() < 1) return;

    const auto &tempDownPath = paths_Down;
    if(_writeBuff->isSolidSplicing())
    {
        Paths pathTarget;
        int nScanner = vecSplicingArea.size();
        for(int iScanner = 0; iScanner < nScanner; ++ iScanner)
        {
            getIntersectionPaths(tempDownPath, vecSplicingArea.at(iScanner).pathArea, pathTarget);
            writeDownFileData_p(pathTarget, nSurfaceIndex, fSpeedRatio, fSpacingRatio, iScanner);
        }
    }
    else
    {
        if (_downsurfaceDisInfoVec.size() > 1)
        {
            long long index = 0;
            for (const auto &path : qAsConst(_distributionDownsurfaceVec))
            {
                Paths tempPaths;
                getIntersectionPaths(tempDownPath, path, tempPaths);
                writeDownFileData_p(tempPaths, nSurfaceIndex, fSpeedRatio, fSpacingRatio,
                                    _downsurfaceDisInfoVec[index]._containorIndex);
                ++ index;
            }
        }
        else
        {
            auto tempIndex = curScannerIndex;
            if (1 == _downsurfaceDisInfoVec.size()) tempIndex = _downsurfaceDisInfoVec.front()._containorIndex;
            writeDownFileData_p(tempDownPath, nSurfaceIndex, fSpeedRatio, fSpacingRatio, tempIndex);
        }
    }
}

void AlgorithmApplication::writeUpFileData(Paths &paths_Up, const int &nSurfaceIndex, const double &fSpeedRatio, const double &fSpacingRatio)
{
    DOWNSURFACEPARAMETERS *lpParas = &BppParas->sSurfacePara_Up;
    if(lpParas->nNumber < 1 || paths_Up.size() < 1) return;

    const auto &tempUpPath = paths_Up;
    if(_writeBuff->isSolidSplicing())
    {
        Paths pathTarget;
        int nScanner = vecSplicingArea.size();
        for(int iScanner = 0; iScanner < nScanner; ++ iScanner)
        {
            getIntersectionPaths(tempUpPath, vecSplicingArea.at(iScanner).pathArea, pathTarget);
            writeUpFileData_p(pathTarget, nSurfaceIndex, fSpeedRatio, fSpacingRatio, iScanner);
        }
    }
    else
    {
        if (_upsurfaceDisInfoVec.size() > 1)
        {
            long long index = 0;
            for (const auto &path : qAsConst(_distributionUpsurfaceVec))
            {
                Paths tempPaths;
                getIntersectionPaths(tempUpPath, path, tempPaths);
                writeUpFileData_p(tempPaths, nSurfaceIndex, fSpeedRatio, fSpacingRatio,
                                  _upsurfaceDisInfoVec[index]._containorIndex);
                ++ index;
            }
        }
        else
        {
            auto tempIndex = curScannerIndex;
            if (1 == _upsurfaceDisInfoVec.size()) tempIndex = _upsurfaceDisInfoVec.front()._containorIndex;
            writeUpFileData_p(tempUpPath, nSurfaceIndex, fSpeedRatio, fSpacingRatio, tempIndex);
        }
    }
}

void AlgorithmApplication::writeSmallStruct(Paths &paths_Small, const double &fSpeedRatio, const double &fSpacingRatio,
                                            const int &BeamIndex, const CB_WRITEDATA &func, const int &areaType)
{
    if(paths_Small.size() < 1) return;

    if(_writeBuff->isSolidSplicing())
    {
        Paths pathTarget;

        int nScanner = vecSplicingArea.size();
        for(int iScanner = 0; iScanner < nScanner; ++ iScanner)
        {
            getIntersectionPaths(paths_Small, vecSplicingArea.at(iScanner).pathArea, pathTarget);
            writeSmallStruct_p(pathTarget, fSpeedRatio, fSpacingRatio, iScanner, BeamIndex, func);
        }
    }
    else
    {
        int scannerIndex = 0;
        if (areaType == DistAreaType::Hatching && _solidDisInfoVec.size()) scannerIndex = _solidDisInfoVec.front()._containorIndex;
        if (areaType == DistAreaType::Upface && _upsurfaceDisInfoVec.size()) scannerIndex = _upsurfaceDisInfoVec.front()._containorIndex;
        if (areaType == DistAreaType::Downface && _downsurfaceDisInfoVec.size()) scannerIndex = _downsurfaceDisInfoVec.front()._containorIndex;
        writeSmallStruct_p(paths_Small, fSpeedRatio, fSpacingRatio, scannerIndex, BeamIndex, func);
    }
}

void AlgorithmApplication::writeSupportToSList(QList<AREAINFOPTR> &listArea, const double &fSpeedRatio)
{
#ifdef CALC_SUPPORT
    if(listArea.size() < 1) return;

    if(_writeBuff->isSupportSplicing())
    {
        if(EXPORT_SLM_BUILDFILE == nExportFileMode)
        {
            Paths supportPaths;
            writeAreaToPaths(listArea, supportPaths);
            QVector<UFFWRITEDATA> vecUFFData;
            QVector<IntPoint> recordCoor;
            for(int i = 0; i < BpcParas->nScannerNumber; ++ i)
            {
                vecUFFData << UFFWRITEDATA();
                recordCoor << IntPoint();
            }
            SplicingPtr->splitSupportData(SplicingPtr->getLayerSplicing(_layerHei), supportPaths,
                                          [fSpeedRatio, &vecUFFData, this](const int &index, const Paths &paths) {
                                              auto tempPaths = std::move(paths);
                                              if(BpcParas->nOptimize_1) scanLinesSortor->sortPaths(tempPaths);
                                              writePathsToScanLines(tempPaths, vecUFFData[index].listSLines);
                                              writeSupportToSList(vecUFFData[index], fSpeedRatio, index);
                                          });
        }
        else writeSupportToSList_p(&listArea, fSpeedRatio, [this](const int64_t &x1, const int64_t &y1, const int64_t &x2,
                                                                 const int64_t &y2, QVector<UFFWRITEDATA> &vecUFFdata) {
                splitLineByVerLine(x1, y1, x2, y2, vecUFFdata);
            });
    }
    else
    {
        UFFWRITEDATA mUFileData;
        if (BpcParas->nOptimize_1)
        {
            Paths totalPaths;
            writeAreaToPaths(listArea, totalPaths);
            scanLinesSortor->sortPaths(totalPaths);
            writePathsToScanLines(totalPaths, mUFileData.listSLines);
        }
        else parsingDatas(listArea, mUFileData.listSLines);
        writeSupportToSList(mUFileData, fSpeedRatio, curScannerIndex);
    }
#endif
}

void AlgorithmApplication::writeBorderData(const Paths &paths_exceptHolesAndGaps, const double &fSpeedRatio)
{
#ifdef CALC_BORDER
    BORDERPARAMETERS *lpParas = &BppParas->sBorderPara;
    if(lpParas->nNumber < 1) return;
    Paths filteSmallB_remaind;
    if(lpParas->nUseSmallBorderFilter)
    {
        Paths paths;
        filteSmallBorder(paths_exceptHolesAndGaps, filteSmallB_remaind, paths,
                         lpParas->fFilterMaxBorderArea *UNITSPRECISION * UNITSPRECISION);
    }
    else
    {
        filteSmallB_remaind = paths_exceptHolesAndGaps;
    }
    if(filteSmallB_remaind.size())
    {
        int nContourTimes = lpParas->nNumber;
        Paths tempPS;
        while(nContourTimes)
        {
            nContourTimes --;
            int nOffsetDis = 0;
            for(int iBorder = 1; iBorder <= nContourTimes; ++ iBorder)
            {
                nOffsetDis += qRound(lpParas->fOffset[iBorder] * FILEDATAUNIT);
            }
            if(0 != nOffsetDis)
            {
                getOffsetPaths(&filteSmallB_remaind, &tempPS, nOffsetDis, 2, 0);
            }
            else
            {
                tempPS = filteSmallB_remaind;
            }

            if(false == _writeBuff->isSolidSplicing())
            {
                if(0 == nContourTimes)
                {
                    getNewBegin(&tempPS);
                }
                writeBorderData(tempPS, nContourTimes, fSpeedRatio, curScannerIndex);
            }
            else
            {
                if(EXPORT_SLM_BUILDFILE == nExportFileMode)
                {
                    auto closeBorder = ExtendedParas<int>("Splicing/nCloseBorder", 0);
                    SplicingPtr->splitBorderData(SplicingPtr->getLayerSplicing(_layerHei), tempPS,
                                                 [=](const int &index, const Paths &paths) {
                                                     auto tempPaths = std::move(paths);
                                                     writeBorderData_splicing(tempPaths, nContourTimes, fSpeedRatio, index);
                                                 }, closeBorder,
                                                 [this, nContourTimes](Paths &paths) { if (0 == nContourTimes) getNewBegin(&paths); });
                }
                else
                {
                    calcLineOnSplicing(tempPS);
                    Paths pathTarget;
                    int nScanner = vecSplicingArea.size();
                    for(int iScanner = 0; iScanner < nScanner; ++ iScanner)
                    {
                        getIntersectionPaths(tempPS, vecSplicingArea.at(iScanner).pathArea, pathTarget);
                        if(0 == nContourTimes)
                        {
                            getNewBegin(&pathTarget);
                        }
                        writeBorderData(pathTarget, nContourTimes, fSpeedRatio, iScanner);
                    }
                }
            }
        }
    }
#endif
}

void AlgorithmApplication::writeExternedBorder(Paths &paths, const double &fSpeedRatio)
{
#ifdef CALC_EXTERNEDBORDER
    if(paths.size() < 1) return;

    if(_writeBuff->isSolidSplicing())
    {
        calcLineOnSplicing(paths);
        Paths pathTarget;
        int nScanner = vecSplicingArea.size();
        for(int iScanner = 0; iScanner < nScanner; ++ iScanner)
        {
            getIntersectionPaths(paths, vecSplicingArea.at(iScanner).pathArea, pathTarget);
            writeExternedBorder_p(pathTarget, fSpeedRatio, iScanner);
        }
    }
    else
    {
        writeExternedBorder_p(paths, fSpeedRatio, curScannerIndex);
    }
#endif
}

void AlgorithmApplication::writeBorderData(Paths &tempPS, const int &nContourTimes, const double &fSpeedRatio, const int &nScanner)
{
    BORDERPARAMETERS *lpParas = &BppParas->sBorderPara;
    UFFWRITEDATA mUFileData;
    mUFileData.nMode_Section = SECTION_POLYGON;
    mUFileData.nLaserPower = BppParas->sGeneralPara.nLaserPower
                                 [lpParas->nIndex_Power[nContourTimes]];
    mUFileData.nMarkSpeed = qRound(lpParas->nMarkSpeed[nContourTimes] * BpcParas->fFactor_2);
    mUFileData.nMarkSpeed = qRound(mUFileData.nMarkSpeed * fSpeedRatio);
    mUFileData.nMode_Coor = SECTION_POLYGONCOOR;

    auto tempIndex = nScanner;
    if (_borderDisInfoVec.size()) tempIndex = _borderDisInfoVec.front()._containorIndex;

    if(vecSplicingLine.size() < 1)
    {
        uint nAreaCnt = tempPS.size();
        for(uint iAreaIndex = 0; iAreaIndex < nAreaCnt; iAreaIndex ++)
        {
            if(EXPORT_SLM_BUILDFILE == nExportFileMode) mUFileData.listSLines.clear();

            Path *lpTempP = nullptr;

            Utek::ReturnPathPack rtnPath;
            if(0 == nContourTimes && lpBEnhancedParas->changerType)
            {
                Utek::PolygonStartChanger borderChanger;
                rtnPath = borderChanger.getStartChanger(tempPS.at(iAreaIndex), *lpBEnhancedParas.data());
                if(rtnPath.successFirst)
                {
                    lpTempP = &rtnPath.result;
                }
                else
                {
                    lpTempP = &tempPS.at(iAreaIndex);
                }
            }
            else
            {
                lpTempP = &tempPS.at(iAreaIndex);
            }

            uint nPtCnt = lpTempP->size();
            if(nPtCnt > 1)
            {
                if(rtnPath.successFirst)
                {
                    scanner_jumpPos(rtnPath.firstPt.X, rtnPath.firstPt.Y, mUFileData.listSLines);
                    scanner_outLine(lpTempP->at(0).X, lpTempP->at(0).Y, mUFileData.listSLines);
                }
                else
                {
                    scanner_jumpPos(int(lpTempP->at(0).X), int(lpTempP->at(0).Y), mUFileData.listSLines);
                }

                for(uint iPt = 1; iPt < nPtCnt; iPt ++)
                {
                    scanner_outLine(lpTempP->at(iPt).X, lpTempP->at(iPt).Y, mUFileData.listSLines);
                }
                scanner_outLine(lpTempP->at(0).X, lpTempP->at(0).Y, mUFileData.listSLines);

                if(rtnPath.successLast)
                {
                    scanner_outLine(rtnPath.lastPt.X, rtnPath.lastPt.Y, mUFileData.listSLines);
                }
            }
            if(EXPORT_SLM_BUILDFILE == nExportFileMode) _writeBuff->appendFileData(mUFileData, tempIndex, lpParas->nIndex_BeamSize[nContourTimes]);
        }
        if(EXPORT_NORMAL == nExportFileMode)
        {
            _writeBuff->appendFileData(mUFileData, tempIndex, lpParas->nIndex_BeamSize[nContourTimes]);
        }
    }
    else
    {
        uint nAreaCnt = tempPS.size();
        for(uint iAreaIndex = 0; iAreaIndex < nAreaCnt; iAreaIndex ++)
        {
            Path *lpTempP = nullptr;
            Utek::ReturnPathPack rtnPath;
            if(0 == nContourTimes && lpBEnhancedParas->changerType)
            {
                Utek::PolygonStartChanger borderChanger;
                rtnPath = borderChanger.getStartChanger(tempPS.at(iAreaIndex), *lpBEnhancedParas.data());
                if(rtnPath.successFirst)
                {
                    lpTempP = &rtnPath.result;
                }
                else
                {
                    lpTempP = &tempPS.at(iAreaIndex);
                }
            }
            else
            {
                lpTempP = &tempPS.at(iAreaIndex);
            }
            uint nPtCnt = lpTempP->size();
            if(nPtCnt > 1)
            {
                if(rtnPath.successFirst)
                {
                    scanner_jumpPos(rtnPath.firstPt.X, rtnPath.firstPt.Y, mUFileData.listSLines);
                    scanner_outLine(lpTempP->at(0).X, lpTempP->at(0).Y, mUFileData.listSLines);
                }
                else
                {
                    scanner_jumpPos(lpTempP->at(0).X, lpTempP->at(0).Y, mUFileData.listSLines);
                }

                int64_t nLastX = lpTempP->at(0).X;
                int64_t nLastY = lpTempP->at(0).Y;
                for(uint iPt = 1; iPt < nPtCnt; iPt ++)
                {
                    if((abs(nLastX - lpTempP->at(iPt).X) < SPLICING_ERROR_HORx2) &&
                        !lineOnVecSplicingLine(nLastX, nLastY, lpTempP->at(iPt).X, lpTempP->at(iPt).Y, vecSplicingLine))
                    {
                        scanner_jumpPos(lpTempP->at(iPt).X, lpTempP->at(iPt).Y, mUFileData.listSLines);
                    }
                    else
                    {
                        scanner_outLine(lpTempP->at(iPt).X, lpTempP->at(iPt).Y, mUFileData.listSLines);
                    }
                    nLastX = lpTempP->at(iPt).X;
                    nLastY = lpTempP->at(iPt).Y;
                }
                if((abs(nLastX - lpTempP->at(0).X) < SPLICING_ERROR_HORx2) &&
                    !lineOnVecSplicingLine(nLastX, nLastY, lpTempP->at(0).X, lpTempP->at(0).Y, vecSplicingLine))
                {
                }
                else
                {
                    scanner_outLine(lpTempP->at(0).X, lpTempP->at(0).Y, mUFileData.listSLines);
                }
                if(rtnPath.successLast)
                {
                    scanner_outLine(rtnPath.lastPt.X, rtnPath.lastPt.Y, mUFileData.listSLines);
                }
            }
        }
        _writeBuff->appendFileData(mUFileData, tempIndex, lpParas->nIndex_BeamSize[nContourTimes]);
    }
}

void AlgorithmApplication::writeBorderData_splicing(Paths &tempPS, const int &nContourTimes, const double &fSpeedRatio, const int &nScanner)
{
    BORDERPARAMETERS *lpParas = &BppParas->sBorderPara;
    UFFWRITEDATA mUFileData;
    mUFileData.nMode_Section = SECTION_POLYGON;
    mUFileData.nLaserPower = BppParas->sGeneralPara.nLaserPower
                                 [lpParas->nIndex_Power[nContourTimes]];
    mUFileData.nMarkSpeed = qRound(lpParas->nMarkSpeed[nContourTimes] * BpcParas->fFactor_2);
    mUFileData.nMarkSpeed = qRound(mUFileData.nMarkSpeed * fSpeedRatio);
    mUFileData.nMode_Coor = SECTION_POLYGONCOOR;

    if(EXPORT_SLM_BUILDFILE == nExportFileMode)
    {
        auto closeBorder = ExtendedParas<int>("Splicing/nCloseBorder", 0);
        for(const auto &path : tempPS)
        {
            mUFileData.listSLines.clear();
            writePathsToScanLines(path, mUFileData.listSLines, closeBorder);
            _writeBuff->appendFileData(mUFileData, nScanner, lpParas->nIndex_BeamSize[nContourTimes]);
        }
    }
    else
    {
        mUFileData.listSLines.clear();
        writePathsToScanLines(tempPS, mUFileData.listSLines, false);
        _writeBuff->appendFileData(mUFileData, nScanner, lpParas->nIndex_BeamSize[nContourTimes]);
    }
}

void AlgorithmApplication::writeSolidSupportBorderData(const Paths &tempPS, const int &nContourTimes, const double &fSpeedRatio, const int &nScanner)
{
    if(0 == ExtendedParas<int>("SolidSupportBorder/nUseBorder", 1)) return;

    //    BORDERPARAMETERS *lpParas = &BppParas->sBorderPara;
    UFFWRITEDATA mUFileData;
    mUFileData.nMode_Section = SECTION_SSPOLYGON;
    mUFileData.nLaserPower = ExtendedParas<int>("SolidSupportBorder/nPower", 80);
    mUFileData.nMarkSpeed = ExtendedParas<int>("SolidSupportBorder/nMarkSpeed", 1000);
    mUFileData.nMode_Coor = SECTION_SSPOLYGONCOOR;

    uint nAreaCnt = tempPS.size();
    for(uint iAreaIndex = 0; iAreaIndex < nAreaCnt; iAreaIndex ++)
    {
        if(EXPORT_SLM_BUILDFILE == nExportFileMode) mUFileData.listSLines.clear();

        const Path *lpTempP = &tempPS.at(iAreaIndex);

        uint nPtCnt = lpTempP->size();
        if(nPtCnt > 1)
        {
            scanner_jumpPos(int(lpTempP->at(0).X), int(lpTempP->at(0).Y), mUFileData.listSLines);
            for(uint iPt = 1; iPt < nPtCnt; iPt ++)
            {
                scanner_outLine(lpTempP->at(iPt).X, lpTempP->at(iPt).Y, mUFileData.listSLines);
            }
            scanner_outLine(lpTempP->at(0).X, lpTempP->at(0).Y, mUFileData.listSLines);
        }
        if(EXPORT_SLM_BUILDFILE == nExportFileMode) _writeBuff->appendFileData(mUFileData, nScanner);
    }
    if(EXPORT_NORMAL == nExportFileMode) _writeBuff->appendFileData(mUFileData, nScanner);
}

void AlgorithmApplication::writeExternedBorder_p(Paths &tempPS, const double &fSpeedRatio, const int &nScanner)
{
    BORDERPARAMETERS *lpBParas = &BppParas->sBorderPara;
    UFFWRITEDATA mUFileData;
    mUFileData.nMode_Section = SECTION_POLYGON;
    mUFileData.nLaserPower = BppParas->sGeneralPara.nLaserPower[lpBParas->nIndex_Power[0]];
    mUFileData.nMarkSpeed = lpBParas->nMarkSpeed[0];
    mUFileData.nMarkSpeed = qRound(mUFileData.nMarkSpeed * fSpeedRatio);
    mUFileData.nMode_Coor = SECTION_POLYGONCOOR;

    auto tempIndex = nScanner;
    if (_borderDisInfoVec.size()) tempIndex = _borderDisInfoVec.front()._containorIndex;

    if(vecSplicingLine.size() < 1)
    {
        uint nAreaCnt = tempPS.size();
        for(uint iAreaIndex = 0; iAreaIndex < nAreaCnt; iAreaIndex ++)
        {
            Path &tempPath = tempPS.at(iAreaIndex);
            uint nPtCnt = tempPath.size();
            if(nPtCnt > 1)
            {
                scanner_jumpPos(tempPath.at(0).X, tempPath.at(0).Y, mUFileData.listSLines);
                for(uint iPt = 1; iPt < nPtCnt; iPt ++)
                {
                    scanner_outLine(tempPath.at(iPt).X, tempPath.at(iPt).Y, mUFileData.listSLines);
                }
                scanner_outLine(tempPath.at(0).X, tempPath.at(0).Y, mUFileData.listSLines);
            }
        }
        _writeBuff->appendFileData(mUFileData, tempIndex, lpBParas->nIndex_BeamSize[HATCHINGBEAM_NORMAL]);
    }
    else
    {
        uint nAreaCnt = tempPS.size();
        for(uint iAreaIndex = 0; iAreaIndex < nAreaCnt; iAreaIndex ++)
        {
            Path &tempPath = tempPS.at(iAreaIndex);
            uint nPtCnt = tempPath.size();
            if(nPtCnt > 1)
            {
                scanner_jumpPos(tempPath.at(0).X, tempPath.at(0).Y, mUFileData.listSLines);
                int64_t nLastX = tempPath.at(0).X;
                int64_t nLastY = tempPath.at(0).Y;
                for(uint iPt = 1; iPt < nPtCnt; iPt ++)
                {
                    if((abs(nLastX - tempPath.at(iPt).X) < SPLICING_ERROR_HORx2) &&
                        !lineOnVecSplicingLine(nLastX, nLastY, tempPath.at(iPt).X, tempPath.at(iPt).Y, vecSplicingLine))
                    {
                        scanner_jumpPos(tempPath.at(iPt).X, tempPath.at(iPt).Y, mUFileData.listSLines);
                    }
                    else
                    {
                        scanner_outLine(tempPath.at(iPt).X, tempPath.at(iPt).Y, mUFileData.listSLines);
                    }
                    nLastX = tempPath.at(iPt).X;
                    nLastY = tempPath.at(iPt).Y;
                }
                if((abs(nLastX - tempPath.at(0).X) < SPLICING_ERROR_HORx2) &&
                    !lineOnVecSplicingLine(nLastX, nLastY, tempPath.at(0).X, tempPath.at(0).Y, vecSplicingLine))
                {
                }
                else
                {
                    scanner_outLine(tempPath.at(0).X, tempPath.at(0).Y, mUFileData.listSLines);
                }
            }
        }
        _writeBuff->appendFileData(mUFileData, tempIndex, lpBParas->nIndex_BeamSize[HATCHINGBEAM_NORMAL]);
    }
}

void AlgorithmApplication::writeHatching_p(const Paths &paths, const double &fSpeedRatio, const double &fSpacingRatio, const int &nScanner, const CB_WRITEDATA &cbFunc)
{
    if(paths.size() < 1) return;
    bool bWriteToFile = true;

    HATCHINGPARAMETERS *lpParas = &BppParas->sHatchingPara;
    UFFWRITEDATA mUFileData;
    mUFileData.nMode_Section = SECTION_HATCH;
    mUFileData.nLaserPower = BppParas->sGeneralPara.nLaserPower[lpParas->nIndex_Power[HATCHINGBEAM_NORMAL]];
    mUFileData.nMarkSpeed = qRound(lpParas->nMarkSpeed[HATCHINGBEAM_NORMAL] * BpcParas->fFactor_4);
    mUFileData.nMarkSpeed = qRound(mUFileData.nMarkSpeed * fSpeedRatio);
    mUFileData.nMode_Coor = SECTION_HATCHCOOR;
    int nLineSpacing = qRound(double(lpParas->fLineSpacing[HATCHINGBEAM_NORMAL]) * FILEDATAUNIT * fSpacingRatio *
                              BpcParas->fFactor_10);
#ifdef USE_PROCESSOR_EXTEND
    nLineSpacing = 700;
#endif
    int nHatchType = BppParas->sHatchingPara.nHatchingType[0];

    switch (nHatchType) {
    default:
    case HATCHTYPE_LINES:
    {
        int nTotalLCnt = 0;
        TOTALHATCHINGLINE totalListHLine;
        calcInnerPoint(paths, totalListHLine, nTotalLCnt, double(fHatchingAngle), nLineSpacing, true);

        int nLConnecter = lpParas->nCLineFactor * nLineSpacing;
        calcSPLine(0 == lpParas->nCLineFactor ? LINETYPE_SORTPATH_GREED : LINETYPE_SORTPATH_TRIANGLE,
                   totalListHLine, mUFileData.listSLines, nTotalLCnt, nLConnecter, lpParas->nCLineFactor);
    }
    break;
    case HATCHTYPE_RING:
        calcRingHatching(paths, mUFileData.listSLines, nLineSpacing);
#ifdef USE_PROCESSOR_EXTEND
        calcSpiralHatching(paths, mUFileData.listSLines, nLineSpacing);
        calcFermatSpiralHatching(paths, mUFileData.listSLines, nLineSpacing);
#endif
        break;
    case HATCHTYPE_SPIRAL:
        calcSpiralHatching(paths, mUFileData.listSLines, nLineSpacing);
        break;
    case HATCHTYPE_FERMATSPIRAL:
        calcFermatSpiralHatching(paths, mUFileData.listSLines, nLineSpacing);
        break;
    case HATCHTYPE_CHECKER:
    {
        bWriteToFile = false;
        double fTempAngle = fHatchingAngle;
        for(const auto &path : qAsConst(getCheckerList()))
        {
            UFFWRITEDATA mUFileDataChecker;
            mUFileDataChecker.nMode_Section = SECTION_HATCH;
            mUFileDataChecker.nLaserPower = BppParas->sGeneralPara.nLaserPower[lpParas->nIndex_Power[HATCHINGBEAM_NORMAL]];
            mUFileDataChecker.nMarkSpeed = qRound(lpParas->nMarkSpeed[HATCHINGBEAM_NORMAL] * BpcParas->fFactor_4);
            mUFileDataChecker.nMarkSpeed = qRound(mUFileDataChecker.nMarkSpeed * fSpeedRatio);
            mUFileDataChecker.nMode_Coor = SECTION_HATCHCOOR;

            Paths targetPath;
            getIntersectionPaths(paths, path, targetPath);
            int nTotalLCnt = 0;
            TOTALHATCHINGLINE totalListHLine;
            calcInnerPoint(targetPath, totalListHLine, nTotalLCnt, fTempAngle, nLineSpacing, true);

            int nLConnecter = lpParas->nCLineFactor * nLineSpacing;
            calcSPLine(0 == lpParas->nCLineFactor ? LINETYPE_SORTPATH_GREED : LINETYPE_SORTPATH_TRIANGLE,
                       totalListHLine, mUFileDataChecker.listSLines, nTotalLCnt, nLConnecter, lpParas->nCLineFactor);

            if(mUFileDataChecker.listSLines.size())
            {
                for(int iTimes = 0; iTimes < lpParas->nScanTimes[HATCHINGBEAM_NORMAL]; ++ iTimes)
                {
                    if (cbFunc._getScanLines) cbFunc._addWriteData(mUFileDataChecker, nScanner, lpParas->nIndex_BeamSize[HATCHINGBEAM_NORMAL]);
                    else _writeBuff->appendFileData(mUFileDataChecker, nScanner, lpParas->nIndex_BeamSize[HATCHINGBEAM_NORMAL]);
                }
                if(scanCheckerBorder()) writeBorderData(targetPath, 0, fSpeedRatio, nScanner);
            }
            fTempAngle += getRotateAngle();
            while(fTempAngle > 360) fTempAngle -= 360;
        }
    }
    break;
    case HATCHTYPE_STRIP:
    {
        bWriteToFile = false;
        double fTempAngle = fHatchingAngle;
        double fTempHatchingAngle = 0.0;
        Paths const stripList = getStripList(fTempAngle, fTempHatchingAngle, BpcParas->mExtendParasObj["WindDirection"].toInt());

        UFFWRITEDATA mUFileDataChecker;
        mUFileDataChecker.nMode_Section = SECTION_HATCH;
        mUFileDataChecker.nLaserPower = BppParas->sGeneralPara.nLaserPower[lpParas->nIndex_Power[HATCHINGBEAM_NORMAL]];
        mUFileDataChecker.nMarkSpeed = qRound(lpParas->nMarkSpeed[HATCHINGBEAM_NORMAL] * BpcParas->fFactor_4);
        mUFileDataChecker.nMarkSpeed = qRound(mUFileDataChecker.nMarkSpeed * fSpeedRatio);
        mUFileDataChecker.nMode_Coor = SECTION_HATCHCOOR;

        for(const auto &path : stripList)
        {
            Paths targetPath;
            QVector<SCANLINE> listSLines;
            getIntersectionPaths(paths, path, targetPath);
            int nTotalLCnt = 0;
            TOTALHATCHINGLINE totalListHLine;
            calcInnerPoint(targetPath, totalListHLine, nTotalLCnt, fTempHatchingAngle, nLineSpacing);

            int nLConnecter = 0;
            calcSPLine(LINETYPE_ORIGINAL,
                       totalListHLine, listSLines, nTotalLCnt, nLConnecter, lpParas->nCLineFactor);
            if(listSLines.size()) mUFileDataChecker.listSLines << listSLines;
        }
        if(mUFileDataChecker.listSLines.size())
        {
            for(int iTimes = 0; iTimes < lpParas->nScanTimes[HATCHINGBEAM_NORMAL]; ++ iTimes)
            {
                if (cbFunc._getScanLines) cbFunc._addWriteData(mUFileDataChecker, nScanner, lpParas->nIndex_BeamSize[HATCHINGBEAM_NORMAL]);
                else _writeBuff->appendFileData(mUFileDataChecker, nScanner, lpParas->nIndex_BeamSize[HATCHINGBEAM_NORMAL]);
            }
        }
        //        fTempAngle += getRotateAngle();
        //        while(fTempAngle > 360) fTempAngle -= 360;
    }
    break;
    }
    if(bWriteToFile)
    {
        if(mUFileData.listSLines.size() < 1) return;
        for(int iTimes = 0; iTimes < lpParas->nScanTimes[HATCHINGBEAM_NORMAL]; ++ iTimes)
        {
            if (cbFunc._getScanLines) cbFunc._addWriteData(mUFileData, nScanner, lpParas->nIndex_BeamSize[HATCHINGBEAM_NORMAL]);
            else _writeBuff->appendFileData(mUFileData, nScanner, lpParas->nIndex_BeamSize[HATCHINGBEAM_NORMAL]);
        }
    }
}

void AlgorithmApplication::writeDownFileData_p(const Paths &pathTarget, const int &nSurfaceIndex, const double &fSpeedRatio,
                                               const double &fSpacingRatio, const int &nScanner)
{
    if(pathTarget.size() < 1) return;

    DOWNSURFACEPARAMETERS *lpParas = &BppParas->sSurfacePara_Dw;
    UFFWRITEDATA mUFileData;
    mUFileData.nMode_Section = SECTION_HATCH_DOWNFILL;
    mUFileData.nLaserPower = BppParas->sGeneralPara.nLaserPower
                                 [lpParas->nIndex_Power[nSurfaceIndex]];
    mUFileData.nMarkSpeed = qRound(lpParas->nMarkSpeed[nSurfaceIndex] * BpcParas->fFactor_6);
    mUFileData.nMarkSpeed = qRound(mUFileData.nMarkSpeed * fSpeedRatio);
    mUFileData.nMode_Coor = SECTION_HATCHDOWNFILLCOOR;

    writeUPDownData(pathTarget, nSurfaceIndex, fSpacingRatio, nScanner, lpParas, mUFileData);
}

void AlgorithmApplication::writeUpFileData_p(const Paths &pathTarget, const int &nSurfaceIndex, const double &fSpeedRatio,
                                             const double &fSpacingRatio, const int &nScanner)
{
    if(pathTarget.size() < 1) return;

    UPSURFACEPARAMETERS *lpParas = &BppParas->sSurfacePara_Up;
    UFFWRITEDATA mUFileData;
    mUFileData.nMode_Section = SECTION_HATCH_UPFILL;
    mUFileData.nLaserPower = BppParas->sGeneralPara.nLaserPower
                                 [lpParas->nIndex_Power[nSurfaceIndex]];
    mUFileData.nMarkSpeed = qRound(lpParas->nMarkSpeed[nSurfaceIndex] * BpcParas->fFactor_5);
    mUFileData.nMarkSpeed = qRound(mUFileData.nMarkSpeed * fSpeedRatio);
    mUFileData.nMode_Coor = SECTION_HATCHUPFILLCOOR;

    writeUPDownData(pathTarget, nSurfaceIndex, fSpacingRatio, nScanner, lpParas, mUFileData);
}

void AlgorithmApplication::writeSmallStruct_p(Paths &pathTarget, const double &fSpeedRatio,
                                              const double &fSpacingRatio, const int &nScanner, const int &BeamIndex, const CB_WRITEDATA &func)
{
    if(pathTarget.size() < 1) return;

    HATCHINGPARAMETERS *lpParas = &BppParas->sHatchingPara;
    UFFWRITEDATA mUFileData;
    mUFileData.nMode_Section = SECTION_HATCH_SMALL;
    mUFileData.nLaserPower = BppParas->sGeneralPara.nLaserPower
                                 [lpParas->nIndex_Power[BeamIndex]];
    mUFileData.nMarkSpeed = qRound(lpParas->nMarkSpeed[BeamIndex] * BpcParas->fFactor_3);
    mUFileData.nMarkSpeed = qRound(mUFileData.nMarkSpeed * fSpeedRatio);
    mUFileData.nMode_Coor = SECTION_HATCHSMALL_COOR;
    TOTALHATCHINGLINE totalListHLine;
    const int nLineSpacing = qRound(double(lpParas->fLineSpacing[BeamIndex]) * FILEDATAUNIT * fSpacingRatio *
                                    BpcParas->fFactor_9);

    int nHatchType = BppParas->sHatchingPara.nHatchingType[BeamIndex];
    switch (nHatchType) {
    default:
    case HATCHTYPE_LINES:
    {
        int nLConnecter = lpParas->nCLineFactor * nLineSpacing;
        int nTotalLCnt = 0;
        calcInnerPoint(pathTarget, totalListHLine, nTotalLCnt, double(fHatchingAngle), nLineSpacing, true);
        //      calcSPLine(LINETYPE_SORTPATH_GREED, totalListHLine, mUFileData.listSLines, nTotalLCnt, nLConnecter);
        calcSPLine(0 == lpParas->nCLineFactor ? LINETYPE_SORTPATH_GREED : LINETYPE_SORTPATH_TRIANGLE,
                   totalListHLine, mUFileData.listSLines, nTotalLCnt, nLConnecter, lpParas->nCLineFactor);
    }
    break;
    case HATCHTYPE_RING:
        calcRingHatching(pathTarget, mUFileData.listSLines, nLineSpacing);
#ifdef USE_PROCESSOR_EXTEND
        calcSpiralHatching(pathTarget, mUFileData.listSLines, nLineSpacing);
        calcFermatSpiralHatching(pathTarget, mUFileData.listSLines, nLineSpacing);
#endif
        break;
    case HATCHTYPE_SPIRAL:
        calcSpiralHatching(pathTarget, mUFileData.listSLines, nLineSpacing);
        break;
    case HATCHTYPE_FERMATSPIRAL:
        calcFermatSpiralHatching(pathTarget, mUFileData.listSLines, nLineSpacing);
        break;
    }
    if (func._getScanLines) func._addWriteData(mUFileData, nScanner, lpParas->nIndex_BeamSize[BeamIndex]);
    else _writeBuff->appendFileData(mUFileData, nScanner, lpParas->nIndex_BeamSize[BeamIndex]);
}

void AlgorithmApplication::writeSupportToSList_p(QList<AREAINFOPTR> *listArea, const double &fSpeedRatio, const FuncSplitScanData &&splitFunc)
{
    QList<AREAINFOPTR> listAreaTemp;
    QVector<UFFWRITEDATA> vecUFFData;

    for(int i = 0; i < vecSplicingArea.size(); ++ i)
    {
        vecUFFData << UFFWRITEDATA();
    }
    int nAreaIndex = 0;
    while(nAreaIndex < listArea->count())
    {
        if(3 == listArea->at(nAreaIndex)->nDir)
        {
            listAreaTemp.append(listArea->takeAt(nAreaIndex));
        }
        else
        {
            nAreaIndex ++;
        }
    }

    int64_t lastX = 0, lastY = 0;
    int64_t curX = 0, curY = 0;

    while(listArea->count())
    {
        Path *lpPath = &listArea->at(0)->ptPath;
        uint nPtCnt = lpPath->size();

        if(nPtCnt)
        {
            lastX = lpPath->at(0).X;
            lastY = lpPath->at(0).Y;
            for(uint iPtIndex = 1; iPtIndex < nPtCnt; ++ iPtIndex)
            {
                curX = lpPath->at(iPtIndex).X;
                curY = lpPath->at(iPtIndex).Y;
                splitFunc(lastX, lastY, curX, curY, vecUFFData);
                lastX = curX;
                lastY = curY;
            }
        }
        listArea->removeAt(0);
    }
    if(listAreaTemp.count())
    {
        int nANum = listAreaTemp.count();
        int iAreaIndex = 0;

        while(iAreaIndex < nANum)
        {
            uint nIndex = 0;
            Path *lpPath = &listAreaTemp[iAreaIndex]->ptPath;
            uint nPtCnt = lpPath->size() >> 1;
            for(uint iNum = 0; iNum < nPtCnt; iNum ++)
            {
                nIndex = iNum << 1;
                splitFunc(lpPath->at(nIndex).X, lpPath->at(nIndex).Y, lpPath->at(nIndex + 1).X,
                          lpPath->at(nIndex + 1).Y, vecUFFData);
            }
            iAreaIndex ++;
        }
    }
    for(int iScanner = 0; iScanner < vecUFFData.size(); ++ iScanner)
    {
        writeSupportToSList(vecUFFData[iScanner], fSpeedRatio, iScanner);
    }
}

void AlgorithmApplication::writeSolidSupportHatching_p(const Paths &paths, const double &fSpeedRatio, const double &fSpacingRatio, const int &nScanner)
{
    if(paths.size() < 1) return;
    bool bWriteToFile = true;

    HATCHINGPARAMETERS *lpParas = &BppParas->sHatchingPara;
    UFFWRITEDATA mUFileData;
    mUFileData.nMode_Section = SECTION_SSHATCH;
    mUFileData.nLaserPower = ExtendedParas<int>("SolidSupportHatching/nPower", 80);
    mUFileData.nMarkSpeed = ExtendedParas<int>("SolidSupportHatching/nMarkSpeed", 1000);
    mUFileData.nMode_Coor = SECTION_SSHATCHCOOR;
    int nLineSpacing = qRound(ExtendedParas<double>("SolidSupportHatching/fLineSpacing", 0.01)
                              * FILEDATAUNIT * fSpacingRatio);
#ifdef USE_PROCESSOR_EXTEND
    nLineSpacing = 700;
#endif
    int nHatchType = ExtendedParas<int>("SolidSupportHatching/nHatchingType", 0);

    switch (nHatchType) {
    default:
    case HATCHTYPE_LINES:
    {
        int nTotalLCnt = 0;
        TOTALHATCHINGLINE totalListHLine;
        calcInnerPoint(paths, totalListHLine, nTotalLCnt, double(fSolidSupportHatchingAngle), nLineSpacing, true);

        int nLConnecter = lpParas->nCLineFactor * nLineSpacing;
        calcSPLine(LINETYPE_SORTPATH_GREED, totalListHLine, mUFileData.listSLines, nTotalLCnt, nLConnecter, 0);
    }
    break;
    case HATCHTYPE_RING:
        calcRingHatching(paths, mUFileData.listSLines, nLineSpacing);
#ifdef USE_PROCESSOR_EXTEND
        calcSpiralHatching(paths, mUFileData.listSLines, nLineSpacing);
        calcFermatSpiralHatching(paths, mUFileData.listSLines, nLineSpacing);
#endif
        break;
    case HATCHTYPE_SPIRAL:
        calcSpiralHatching(paths, mUFileData.listSLines, nLineSpacing);
        break;
    case HATCHTYPE_FERMATSPIRAL:
        calcFermatSpiralHatching(paths, mUFileData.listSLines, nLineSpacing);
        break;
    case HATCHTYPE_CHECKER:
    {
        bWriteToFile = false;
        double fTempAngle = fSolidSupportHatchingAngle;
        for(const auto &path : qAsConst(getCheckerList()))
        {
            UFFWRITEDATA mUFileDataChecker;
            mUFileDataChecker.nMode_Section = SECTION_SSHATCH;
            mUFileData.nLaserPower = ExtendedParas<int>("SolidSupportHatching/nPower", 80);
            mUFileData.nMarkSpeed = ExtendedParas<int>("SolidSupportHatching/nMarkSpeed", 1000);
            mUFileDataChecker.nMode_Coor = SECTION_SSHATCHCOOR;

            Paths targetPath;
            getIntersectionPaths(paths, path, targetPath);
            int nTotalLCnt = 0;
            TOTALHATCHINGLINE totalListHLine;
            calcInnerPoint(targetPath, totalListHLine, nTotalLCnt, fTempAngle, nLineSpacing, true);

            calcSPLine(LINETYPE_SORTPATH_GREED, totalListHLine, mUFileDataChecker.listSLines, nTotalLCnt, 0, 0);

            if(mUFileDataChecker.listSLines.size())
            {
                for(int iTimes = 0; iTimes < lpParas->nScanTimes[HATCHINGBEAM_NORMAL]; ++ iTimes)
                {
                    _writeBuff->appendFileData(mUFileDataChecker, nScanner, lpParas->nIndex_BeamSize[HATCHINGBEAM_NORMAL]);
                }
                if(scanCheckerBorder()) writeBorderData(targetPath, 0, fSpeedRatio, nScanner);
            }
            fTempAngle += getRotateAngle();
            while(fTempAngle > 360) fTempAngle -= 360;
        }
    }
    break;
    case HATCHTYPE_STRIP:
    {
        bWriteToFile = false;
        double fTempAngle = fSolidSupportHatchingAngle;
        double fTempHatchingAngle = 0.0;
        Paths const stripList = getStripList(fTempAngle, fTempHatchingAngle, BpcParas->mExtendParasObj["WindDirection"].toInt());

        UFFWRITEDATA mUFileDataChecker;
        mUFileDataChecker.nMode_Section = SECTION_SSHATCH;
        mUFileData.nLaserPower = ExtendedParas<int>("SolidSupportHatching/nPower", 80);
        mUFileData.nMarkSpeed = ExtendedParas<int>("SolidSupportHatching/nMarkSpeed", 1000);
        mUFileDataChecker.nMode_Coor = SECTION_SSHATCHCOOR;

        for(const auto &path : stripList)
        {
            Paths targetPath;
            QVector<SCANLINE> listSLines;
            getIntersectionPaths(paths, path, targetPath);
            int nTotalLCnt = 0;
            TOTALHATCHINGLINE totalListHLine;
            calcInnerPoint(targetPath, totalListHLine, nTotalLCnt, fTempHatchingAngle, nLineSpacing, true);

            int nLConnecter = 0;
            calcSPLine(LINETYPE_SORTPATH_GREED,
                       totalListHLine, listSLines, nTotalLCnt, nLConnecter, 0);

            mUFileDataChecker.listSLines << listSLines;
        }
        if(mUFileDataChecker.listSLines.size())
        {
            for(int iTimes = 0; iTimes < lpParas->nScanTimes[HATCHINGBEAM_NORMAL]; ++ iTimes)
            {
                _writeBuff->appendFileData(mUFileDataChecker, nScanner, lpParas->nIndex_BeamSize[HATCHINGBEAM_NORMAL]);
            }
        }
        //        fTempAngle += getRotateAngle();
        //        while(fTempAngle > 360) fTempAngle -= 360;
    }
    break;
    }
    if(bWriteToFile)
    {
        if(mUFileData.listSLines.size() < 1) return;
        for(int iTimes = 0; iTimes < lpParas->nScanTimes[HATCHINGBEAM_NORMAL]; ++ iTimes)
        {
            _writeBuff->appendFileData(mUFileData, nScanner, lpParas->nIndex_BeamSize[HATCHINGBEAM_NORMAL]);
        }
    }
}

void AlgorithmApplication::writeSupportToSList(UFFWRITEDATA &mUFileData, const double &fSpeedRatio, const int &nScanner)
{
    if(mUFileData.listSLines.size() < 1) return;
    SUPPORTPARAMETERS *lpParas = &BppParas->sSupportPara;
    mUFileData.nMode_Section = SECTION_SUPPORT;
    mUFileData.nLaserPower = BppParas->sGeneralPara.nLaserPower[lpParas->nIndex_Power];
    mUFileData.nMarkSpeed = qRound(lpParas->nMarkSpeed * BpcParas->fFactor_1);
    mUFileData.nMarkSpeed = qRound(mUFileData.nMarkSpeed * fSpeedRatio);
    mUFileData.nMode_Coor = SECTION_SUPPORTCOOR;
    _writeBuff->appendFileData(mUFileData, nScanner, lpParas->nIndex_BeamSize);
}

//private
void AlgorithmApplication::writeUPDownData(const Paths &pathTarget, const int &nIndex, const double &fSpacingRatio, const int &nScanner,
                                           UPSURFACEPARAMETERS *lpSurfaceParas, UFFWRITEDATA &mUFileData)
{
    bool bWriteToFile = true;
    TOTALHATCHINGLINE totalListHLine;
    int nLineSpacing = qRound(double(lpSurfaceParas->fLineSpacing[nIndex]) * FILEDATAUNIT * fSpacingRatio);
    int nTotalLCnt = 0;

    switch(lpSurfaceParas->nHatchingType[nIndex])
    {
    case 0:
        calcInnerPoint(pathTarget, totalListHLine, nTotalLCnt, 0.0, nLineSpacing);
        break;
    case 1:
        calcInnerPoint(pathTarget, totalListHLine, nTotalLCnt, 90.0, nLineSpacing);
        break;
    case 2:
    {
        double fHAngle = (nIndex & 1) ? 90.0 : 0.0;
        calcInnerPoint(pathTarget, totalListHLine, nTotalLCnt, fHAngle, nLineSpacing);
    }
    break;
    case 3:
    default:
        TOTALHATCHINGLINE tempListHLine;
        calcInnerPoint(pathTarget, totalListHLine, nTotalLCnt, 0.0, nLineSpacing);
        calcInnerPoint(pathTarget, tempListHLine, nTotalLCnt, 90.0, nLineSpacing);
        totalListHLine << tempListHLine;
        break;
    }

    int nHatchType = BppParas->sHatchingPara.nHatchingType[0];
    switch(nHatchType)
    {
    case HATCHTYPE_STRIP:
    {
        bWriteToFile = false;
        double fTempAngle = fHatchingAngle;
        double fTempHatchingAngle = 0.0;
        Paths const stripList = getStripList(fTempAngle, fTempHatchingAngle, BpcParas->mExtendParasObj["WindDirection"].toInt());

        UFFWRITEDATA mUFileDataChecker;
        mUFileDataChecker.nMode_Section = SECTION_HATCH;
        mUFileDataChecker.nLaserPower = BppParas->sGeneralPara.nLaserPower[lpSurfaceParas->nIndex_Power[HATCHINGBEAM_NORMAL]];
        mUFileDataChecker.nMarkSpeed = qRound(lpSurfaceParas->nMarkSpeed[HATCHINGBEAM_NORMAL] * BpcParas->fFactor_4);
        mUFileDataChecker.nMarkSpeed = qRound(mUFileDataChecker.nMarkSpeed * 1.0);
        mUFileDataChecker.nMode_Coor = SECTION_HATCHCOOR;

        for(const auto &path : stripList)
        {
            Paths targetPath;
            QVector<SCANLINE> listSLines;
            getIntersectionPaths(pathTarget, path, targetPath);
            int nTotalLCnt = 0;
            TOTALHATCHINGLINE totalListHLine;
            calcInnerPoint(targetPath, totalListHLine, nTotalLCnt, fTempHatchingAngle, nLineSpacing);

            int nLConnecter = 0;

            calcSPLine(LINETYPE_ORIGINAL,
                       totalListHLine, listSLines, nTotalLCnt, nLConnecter, nLineSpacing);
            if(listSLines.size()) mUFileDataChecker.listSLines << listSLines;
        }
        if(mUFileDataChecker.listSLines.size())
        {
            for(int iTimes = 0; iTimes < lpSurfaceParas->nScanTimes[HATCHINGBEAM_NORMAL]; ++ iTimes)
            {
                _writeBuff->appendFileData(mUFileDataChecker, nScanner, lpSurfaceParas->nIndex_BeamSize);
            }
        }
    }
    break;
    }

    if(bWriteToFile)
    {
        calcSPLine(LINETYPE_ORIGINAL, totalListHLine, mUFileData.listSLines);

        if(mUFileData.listSLines.size() < 1) return;
        for(int iTimes = 0; iTimes < lpSurfaceParas->nScanTimes[nIndex]; ++ iTimes)
        {
            _writeBuff->appendFileData(mUFileData, nScanner, lpSurfaceParas->nIndex_BeamSize);
        }
    }
}

void AlgorithmApplication::splitLineByVerLine(const int64_t &x1, const int64_t &y1, const int64_t &x2,
                                              const int64_t &y2, QVector<UFFWRITEDATA> &vecUFFdata)
{
    int nPt1Area = -1;
    int nPt2Area = -1;
    for(int iArea = 0; iArea < vecSplicingArea.size(); ++ iArea)
    {
        if(x1 < vecSplicingArea.at(iArea).nSplicingPosRight)
        {
            nPt1Area = iArea;
            break;
        }
    }
    for(int iArea = 0; iArea < vecSplicingArea.size(); ++ iArea)
    {
        if(x2 < vecSplicingArea.at(iArea).nSplicingPosRight)
        {
            nPt2Area = iArea;
            break;
        }
    }

    if(-1 == nPt1Area || -1 == nPt2Area)
    {
        for(int iArea = 0; iArea < vecSplicingArea.size(); ++ iArea)
        {
            qDebug() << "splitLine Error!" << iArea << vecSplicingArea.at(iArea).nSplicingPosRight;
        }
        qDebug() << "splitLine Error!" << x1 << y1 << x2 << y2 << nPt1Area << nPt2Area;
        return;
    }
    if(nPt1Area == nPt2Area)
    {
        writeDataToList(x1, y1, x2, y2, vecSplicingArea[nPt1Area].nLastX,
                        vecSplicingArea[nPt1Area].nLastY, vecUFFdata[nPt1Area].listSLines);
    }
    else
    {
        if(nPt1Area < nPt2Area)
        {
            int64_t nTempX = x1;
            int64_t nTempY = y1;
            for(int iArea = nPt1Area; iArea < nPt2Area; ++ iArea)
            {
                int64_t nXPos = vecSplicingArea[iArea].nSplicingPosRight;
                int64_t nYPos = y1 + (nXPos - x1) * (y2 - y1) / (x2 - x1);
                writeDataToList(nTempX, nTempY, nXPos, nYPos, vecSplicingArea[iArea].nLastX,
                                vecSplicingArea[iArea].nLastY, vecUFFdata[iArea].listSLines);
                nTempX = nXPos;
                nTempY = nYPos;
            }
            writeDataToList(nTempX, nTempY, x2, y2, vecSplicingArea[nPt2Area].nLastX,
                            vecSplicingArea[nPt2Area].nLastY, vecUFFdata[nPt2Area].listSLines);
        }
        else
        {
            int64_t nTempX = x1;
            int64_t nTempY = y1;
            for(int iArea = nPt1Area; iArea > nPt2Area; -- iArea)
            {
                int64_t nXPos = vecSplicingArea[iArea].nSplicingPosLeft;
                int64_t nYPos = y1 + (nXPos - x1) * (y2 - y1) / (x2 - x1);
                writeDataToList(nTempX, nTempY, nXPos, nYPos, vecSplicingArea[iArea].nLastX,
                                vecSplicingArea[iArea].nLastY, vecUFFdata[iArea].listSLines);
                nTempX = nXPos;
                nTempY = nYPos;
            }
            writeDataToList(nTempX, nTempY, x2, y2, vecSplicingArea[nPt2Area].nLastX,
                            vecSplicingArea[nPt2Area].nLastY, vecUFFdata[nPt2Area].listSLines);
        }
    }
}

int AlgorithmApplication::getUpDownMinWall(DOWNSURFACEPARAMETERS *lpParas)
{
    return qRound(double(BppParas->sHatchingPara.fMinWall[BppParas->sGeneralPara.nNumber_Beam - lpParas->nIndex_BeamSize - 1]) *
                  FILEDATAUNIT_0_5);
}

void AlgorithmApplication::writeAreaToPaths(QList<AREAINFOPTR> &listArea, Paths &paths)
{
    paths.clear();
    int nDir = 0;
    for (const auto &areaInfo : qAsConst(listArea))
    {
        nDir = areaInfo->nDir;
        if (0 == nDir || 1 == nDir)
        {
            auto tempPath = areaInfo->ptPath;
            if (tempPath.size())
            {
                tempPath.push_back(tempPath.front());
                paths << tempPath;
            }
        }
        else if (3 == nDir)
        {
            if (areaInfo->ptPath.size() && 0 == (areaInfo->ptPath.size() % 2))
            {
                auto ptCnt = areaInfo->ptPath.size();
                for (auto i = 0; i < ptCnt; i += 2)
                {
                    paths.push_back({{areaInfo->ptPath[i]}, {areaInfo->ptPath[i + 1]}});
                }
            }
        }
        else if (areaInfo->ptPath.size()) paths << areaInfo->ptPath;
    }
}

void AlgorithmApplication::parsingDatas(QList<AREAINFOPTR> &listArea, QVector<SCANLINE> &listSLines)
{
    uint PNum = 0;
    int ANum = listArea.count();
    int nDir = 0;
    int iAreaNum = 0;
    while(iAreaNum < ANum)
    {
        nDir = listArea.at(iAreaNum)->nDir;
        PNum = listArea.at(iAreaNum)->ptPath.size();
        switch(nDir)
        {
        case 0:
        case 1:
            scanner_jumpPos(int(listArea.at(iAreaNum)->ptPath[0].X),
                            int(listArea.at(iAreaNum)->ptPath[0].Y), listSLines);
            for(uint iNum = 1; iNum < PNum; iNum ++)
            {
                scanner_outLine(int(listArea.at(iAreaNum)->ptPath[iNum].X),
                                int(listArea.at(iAreaNum)->ptPath[iNum].Y), listSLines);
            }
            scanner_outLine(int(listArea.at(iAreaNum)->ptPath[0].X),
                            int(listArea.at(iAreaNum)->ptPath[0].Y), listSLines);
            break;
        case 2:
            scanner_jumpPos(int(listArea.at(iAreaNum)->ptPath[0].X),
                            int(listArea.at(iAreaNum)->ptPath[0].Y), listSLines);
            for(uint iNum = 1; iNum < PNum; iNum ++)
            {
                scanner_outLine(int(listArea.at(iAreaNum)->ptPath[iNum].X),
                                int(listArea.at(iAreaNum)->ptPath[iNum].Y), listSLines);
            }
            break;
        case 3:
        default:
            uint nLineCnt = PNum >> 1;
            uint nIndex = 0;
            for(uint iLine = 0; iLine < nLineCnt; iLine ++)
            {
                nIndex = iLine << 1;
                scanner_plotLine(int(listArea.at(iAreaNum)->ptPath[nIndex].X),
                                 int(listArea.at(iAreaNum)->ptPath[nIndex].Y),
                                 int(listArea.at(iAreaNum)->ptPath[nIndex + 1].X),
                                 int(listArea.at(iAreaNum)->ptPath[nIndex + 1].Y), listSLines);
            }
            break;
        }
        iAreaNum ++;
    }
}

void AlgorithmApplication::getNewBegin(Paths *lpPaths)
{
    if(false == BppParas->sEnhancedBorder.bUseRandomBegin) return;

    for(uint iPath = 0; iPath < lpPaths->size(); iPath ++)
    {
        Path tempP = lpPaths->at(iPath);
        uint nPtCnt = tempP.size();
        if(nPtCnt)
        {
            uint nRandPt = uint(qrand()) % nPtCnt;
            uint iPt = 0;
            while(iPt < nPtCnt)
            {
                lpPaths->at(iPath).at(iPt).X = tempP.at(nRandPt).X;
                lpPaths->at(iPath).at(iPt).Y = tempP.at(nRandPt).Y;
                iPt ++;
                nRandPt ++;
                nRandPt = nRandPt > (nPtCnt - 1) ? 0 : nRandPt;
            }
        }
    }
}

void AlgorithmApplication::calcLineOnSplicing(Paths &tempPath)
{
    for(int i = 0; i < vecSplicingLine.size(); ++ i)
    {
        vecSplicingLine[i].vecLine.clear();
        calcLineOnSplicing(tempPath, vecSplicingLine[i]);
    }
}

void AlgorithmApplication::calcLineOnSplicing(Paths &tempPath, SplicingLine &splicingLine)
{
    uint nAreaCnt = tempPath.size();
    for(uint iAreaIndex = 0; iAreaIndex < nAreaCnt; iAreaIndex ++)
    {
        Path *lpTempP = &tempPath.at(iAreaIndex);
        uint nPtCnt = lpTempP->size();
        for(uint iPt = 1; iPt < nPtCnt; iPt ++)
        {
            if((abs(splicingLine.nSplicingPos - lpTempP->at(iPt - 1).X) < SPLICING_ERROR_HOR) &&
                (abs(splicingLine.nSplicingPos - lpTempP->at(iPt).X) < SPLICING_ERROR_HOR))
            {
                splicingLine.vecLine << LINENORMAL(lpTempP->at(iPt - 1).X, lpTempP->at(iPt - 1).Y,
                                                   lpTempP->at(iPt).X, lpTempP->at(iPt).Y);
            }
        }
        if(nPtCnt > 2)
        {
            if((abs(splicingLine.nSplicingPos - lpTempP->at(nPtCnt - 1).X) < SPLICING_ERROR_HOR) &&
                (abs(splicingLine.nSplicingPos - lpTempP->at(0).X) < SPLICING_ERROR_HOR))
            {
                splicingLine.vecLine << LINENORMAL(lpTempP->at(nPtCnt - 1).X, lpTempP->at(nPtCnt - 1).Y,
                                                   lpTempP->at(0).X, lpTempP->at(0).Y);
            }
        }
    }
}

void AlgorithmApplication::keepHolesAndGaps(Paths *lpPaths, Paths &pathHoles, Paths &pathGaps, Paths &pathExcept)
{
#ifdef CALC_SMALLGAP_SMALLHOLE
    BPPBUILDPARAMETERS *lpBppParas = BppParas.data();
    uint iAreaIndex = 0;
    Paths tempPaths_Src, tempPaths;
    PATHPARAMETERS tempPathPara;

    pathHoles.clear();
    pathGaps.clear();
    pathExcept.clear();

    Paths lpHoles[3], lpGaps[3];
    lpHoles[0].clear();
    lpHoles[1].clear();
    lpHoles[2].clear();
    lpGaps[0].clear();
    lpGaps[1].clear();
    lpGaps[2].clear();

    bool bNormalBorder = true;
    while(iAreaIndex < lpPaths->size())
    {
        bNormalBorder = true;
        Path *lpTempP = &lpPaths->at(iAreaIndex);
        uint nPtCnt = lpTempP->size();
        double fArea = Area(*lpTempP);
        if((fArea < 0) && (nPtCnt > 2))
        {
            tempPathPara.initPath(*lpTempP);
            for(int iSection = 0; iSection < lpBppParas->sSmallHolePara.nNumber_Section; iSection ++)
            {
                if(tempPathPara.isHolesAndLessRadius(qRound(lpBppParas->sSmallHolePara.fRadius[iSection] * FILEDATAUNIT), fabs(fArea)))
                {
                    lpHoles[iSection] << lpPaths->at(iAreaIndex);
                    bNormalBorder = false;
                    break;
                }
            }
            if(bNormalBorder)
            {
                tempPaths_Src.clear();
                tempPaths_Src << *lpTempP;
                SimplifyPolygons(tempPaths_Src);
                for(int iSection = 0; iSection < lpBppParas->sSmallGapPara.nNumber_Section; iSection ++)
                {
                    getOffsetPaths(&tempPaths_Src, &tempPaths, 0,
                                   qRound(double(lpBppParas->sSmallGapPara.fWidth[iSection]) * FILEDATAUNIT_0_5), FILEDATAUNIT_0_05);
                    if(0 == tempPaths.size())
                    {
                        tempPathPara.initPath(*lpTempP);
                        if(tempPathPara.isLittleGap(qRound(lpBppParas->sSmallGapPara.fWidth[iSection] * FILEDATAUNIT), fArea))
                        {
                            lpGaps[iSection] << lpPaths->at(iAreaIndex);
                            bNormalBorder = false;
                            break;
                        }
                    }
                }
            }
        }
        if(bNormalBorder)
        {
            iAreaIndex ++;
        }
        else
        {
            lpPaths->erase(lpPaths->begin() + int(iAreaIndex));
        }
    }
    pathExcept = *lpPaths;
    for(int iSection = 0; iSection < lpBppParas->sSmallHolePara.nNumber_Section; iSection ++)
    {
        if(lpHoles[iSection].size())
        {
            SimplifyPolygons(lpHoles[iSection]);
            getOffsetPaths(&lpHoles[iSection], - qRound(lpBppParas->sSmallHolePara.fOffset[iSection] * FILEDATAUNIT), 0, int(FILEDATAUNIT_0_05));
            for(uint iIndex = 0; iIndex < lpHoles[iSection].size(); ++ iIndex)
            {
                *lpPaths << lpHoles[iSection].at(iIndex);
                pathHoles << lpHoles[iSection].at(iIndex);
            }
        }
    }
    for(int iSection = 0; iSection < lpBppParas->sSmallGapPara.nNumber_Section; iSection ++)
    {
        if(lpGaps[iSection].size())
        {
            SimplifyPolygons(lpGaps[iSection]);
            getOffsetPaths(&lpGaps[iSection], - qRound(lpBppParas->sSmallGapPara.fOffset[iSection] * FILEDATAUNIT), 0, int(FILEDATAUNIT_0_05));
            for(uint iIndex = 0; iIndex < lpGaps[iSection].size(); ++ iIndex)
            {
                *lpPaths << lpGaps[iSection].at(iIndex);
                pathGaps << lpGaps[iSection].at(iIndex);
            }
        }
    }
    SimplifyPolygons(*lpPaths);
    SimplifyPolygons(pathHoles);
    SimplifyPolygons(pathGaps);
    SimplifyPolygons(pathExcept);
#else
    pathExcept = *lpPaths;
#endif
}

QMap<int, Paths> AlgorithmApplication::splitPathsByFiled(const Paths &paths)
{
    auto pathsMap = QMap<int, Paths>();
    for (const auto &path : paths)
    {
        auto bounds = IntRect::GetBounds(paths);
        auto index  = BpcParas->getCurScannerIndex((bounds.left + bounds.right) * 0.5 * UNITFACTOR,
                                                  (bounds.top + bounds.bottom) * 0.5 * UNITFACTOR);
        pathsMap[index] << path;
    }
    return pathsMap;
}

void AlgorithmApplication::writeSolidSupportData(Paths &solidSupport, const double &borderSpeedFactor,
                                                 const double &fSpeedRatio, const double &fLLineSpace, const double &fSLineSpace)
{
    double fLineSpace = isBigBeam(BppParas->sGeneralPara.nNumber_Beam, BppParas->sGeneralPara.nNumber_Beam - 1) ?
                            fLLineSpace : fSLineSpace;
    SplicingPtr->splitSolidData(SplicingPtr->getLayerSplicing(_layerHei), solidSupport,
                                [=](const int &index, const Paths &paths) {
                                    writeAllHatching(paths, HATCHINGBEAM_NORMAL, fSpeedRatio, fSLineSpace,
                                                     [=](Paths &curPaths, Paths &lastPaths, const CB_WRITEDATA &) {
                                                         HATCHINGPARAMETERS *lpPara = &BppParas->sHatchingPara;
                                                         double fOffset = double(lpPara->fOffset[HATCHINGBEAM_NORMAL]) * FILEDATAUNIT * BpcParas->fFactor_8;
                                                         recalcBeamStruct(paths, lastPaths, curPaths, fOffset);
                                                         writeSolidSupportHatching_p(curPaths, fSpeedRatio, fLineSpace, index);
                                                     });
                                    writeSolidSupportBorderData(paths, 1, borderSpeedFactor, index);
                                });
}

void AlgorithmApplication::writeHoleAndGapData(Paths &holes, Paths &gaps, const double &fSpeedRatio)
{
#ifdef CALC_SMALLGAP_SMALLHOLE
    writeHoleAndGapData(holes, qRound(BppParas->sSmallHolePara.nMarkSpeed * fSpeedRatio));
    writeHoleAndGapData(gaps, qRound(BppParas->sSmallGapPara.nMarkSpeed * fSpeedRatio));
#endif
}

void AlgorithmApplication::writeHoleAndGapData(Paths &lpPaths, const int &nSpeed)
{
    //    soomthBorders(&lpPaths);
    if(lpPaths.size())
    {
        int nContourTimes = BppParas->sBorderPara.nNumber;
        Paths tempPS;
        while(nContourTimes)
        {
            nContourTimes --;
            int nOffsetDis = 0;
            for(int iBorder = 1; iBorder <= nContourTimes; ++ iBorder)
            {
                nOffsetDis += qRound(BppParas->sBorderPara.fOffset[iBorder] * FILEDATAUNIT);
            }
            if(0 != nOffsetDis)
            {
                getOffsetPaths(&lpPaths, &tempPS, - nOffsetDis, 2, 0);
            }
            else
            {
                tempPS = lpPaths;
            }
            if(BpcParas->nNumber_SplicingScanner < 2 ||
                !(BppParas->sGeneralPara.nUseSplicingMode & 0x1))
            {
                if(0 == nContourTimes)
                {
                    getNewBegin(&tempPS);
                }
                writeHoleAndGapData(tempPS, nSpeed, nContourTimes, curScannerIndex);
            }
            else
            {
                calcLineOnSplicing(tempPS);

                Paths pathTarget;
                int nScanner = vecSplicingArea.size();
                for(int iScanner = 0; iScanner < nScanner; ++ iScanner)
                {
                    getIntersectionPaths(tempPS, vecSplicingArea.at(iScanner).pathArea, pathTarget);
                    if(0 == nContourTimes)
                    {
                        getNewBegin(&pathTarget);
                    }
                    writeHoleAndGapData(pathTarget, nSpeed, nContourTimes, iScanner);
                }

            }
        }
    }
}

void AlgorithmApplication::writeHoleAndGapData(Paths &tempPS, const int &nSpeed, const int &nContourTimes,
                                               const int &scanner)
{
    UFFWRITEDATA mUFileData;
    mUFileData.nMode_Section = SECTION_POLYGON;
    mUFileData.nLaserPower = BppParas->sGeneralPara.nLaserPower
                                 [BppParas->sBorderPara.nIndex_Power[nContourTimes]];
    mUFileData.nMarkSpeed = qRound(nSpeed * BpcParas->fFactor_2);
    mUFileData.nMode_Coor = SECTION_POLYGONCOOR;

    auto tempIndex = scanner;
    if (_borderDisInfoVec.size()) tempIndex = _borderDisInfoVec.front()._containorIndex;

    if(vecSplicingLine.size() < 1)
    {
        uint nAreaCnt = tempPS.size();
        for(uint iAreaIndex = 0; iAreaIndex < nAreaCnt; iAreaIndex ++)
        {
            Path *lpTempP = &tempPS.at(iAreaIndex);
            uint nPtCnt = lpTempP->size();
            if(nPtCnt > 1)
            {
                scanner_jumpPos(lpTempP->at(0).X, lpTempP->at(0).Y, mUFileData.listSLines);
                for(uint iPt = 1; iPt < nPtCnt; iPt ++)
                {
                    scanner_outLine(lpTempP->at(iPt).X, lpTempP->at(iPt).Y, mUFileData.listSLines);
                }
                scanner_outLine(lpTempP->at(0).X, lpTempP->at(0).Y, mUFileData.listSLines);
            }
        }
        _writeBuff->appendFileData(mUFileData, tempIndex, BppParas->sBorderPara.nIndex_BeamSize[nContourTimes]);
    }
    else
    {
        uint nAreaCnt = tempPS.size();
        for(uint iAreaIndex = 0; iAreaIndex < nAreaCnt; iAreaIndex ++)
        {
            Path *lpTempP = &tempPS.at(iAreaIndex);
            uint nPtCnt = lpTempP->size();
            if(nPtCnt > 1)
            {
                scanner_jumpPos(lpTempP->at(0).X, lpTempP->at(0).Y, mUFileData.listSLines);
                cInt nLastX = lpTempP->at(0).X;
                cInt nLastY = lpTempP->at(0).Y;
                for(uint iPt = 1; iPt < nPtCnt; iPt ++)
                {
                    if((abs(nLastX - lpTempP->at(iPt).X) < SPLICING_ERROR_HORx2) &&
                        !lineOnVecSplicingLine(nLastX, nLastY, lpTempP->at(iPt).X, lpTempP->at(iPt).Y, vecSplicingLine))
                    {
                        scanner_jumpPos(lpTempP->at(iPt).X, lpTempP->at(iPt).Y, mUFileData.listSLines);
                    }
                    else
                    {
                        scanner_outLine(lpTempP->at(iPt).X, lpTempP->at(iPt).Y, mUFileData.listSLines);
                    }
                    nLastX = lpTempP->at(iPt).X;
                    nLastY = lpTempP->at(iPt).Y;
                }
                if((abs(nLastX - lpTempP->at(0).X) < SPLICING_ERROR_HORx2) &&
                    !lineOnVecSplicingLine(nLastX, nLastY, lpTempP->at(0).X, lpTempP->at(0).Y, vecSplicingLine))
                {
                }
                else
                {
                    scanner_outLine(lpTempP->at(0).X, lpTempP->at(0).Y, mUFileData.listSLines);
                }
            }
        }
        _writeBuff->appendFileData(mUFileData, tempIndex, BppParas->sBorderPara.nIndex_BeamSize[nContourTimes]);
    }
}

bool AlgorithmApplication::createDistributionArea(const Paths &paths, const int &solid_type)
{
    if (paths.size() < 1) return true;

    std::vector<PartResult> *distInfoVec = &_solidDisInfoVec;
    std::vector<Path> *distributionPathVec = &_distributionPathVec;

    switch (solid_type)
    {
    case DistAreaType::Upface:
        distInfoVec = &_upsurfaceDisInfoVec;
        distributionPathVec = &_distributionUpsurfaceVec;
        break;
    case DistAreaType::Downface:
        distInfoVec = &_downsurfaceDisInfoVec;
        distributionPathVec = &_distributionDownsurfaceVec;
        break;
    }

    distributionPathVec->clear();
    if (distInfoVec->size() < 2) return true;

    std::sort(distInfoVec->begin(), distInfoVec->end(),
              [](const PartResult &res1, const PartResult &res2){
                  return res1._factor > res2._factor;
              });

    long long leftPos = 0;
    long long rightPos = 1E7;
    long long bottomPos = 0;
    long long topPos = 1E7;

    double totalFactor = 1.0;

    Path leftPath;
    Paths tempPaths = paths;
    auto splitCnt = distInfoVec->size() - 1;
    for (int i = 0; i < splitCnt; ++ i)
    {
        PolygonsDivider divider;
        int splitHorizon = 0;
        auto pos = divider.calcWeightPos(tempPaths, (*distInfoVec)[i]._factor / totalFactor, splitHorizon);
        totalFactor -= (*distInfoVec)[i]._factor;

        if (totalFactor < 1E-6) return false;
        if (splitHorizon)
        {
            Path tempPath;
            tempPath << IntPoint(leftPos, bottomPos) << IntPoint(rightPos, bottomPos)
                     << IntPoint(rightPos, pos) << IntPoint(leftPos, pos);
            distributionPathVec->push_back(std::move(tempPath));

            bottomPos = pos;
            leftPath.clear();
            leftPath << IntPoint(leftPos, bottomPos) << IntPoint(rightPos, bottomPos)
                     << IntPoint(rightPos, topPos) << IntPoint(leftPos, topPos);
        }
        else
        {
            Path tempPath;
            tempPath << IntPoint(leftPos, bottomPos) << IntPoint(pos, bottomPos)
                     << IntPoint(pos, topPos) << IntPoint(leftPos, topPos);
            distributionPathVec->push_back(std::move(tempPath));

            leftPos = pos;
            leftPath.clear();
            leftPath << IntPoint(leftPos, bottomPos) << IntPoint(rightPos, bottomPos)
                     << IntPoint(rightPos, topPos) << IntPoint(leftPos, topPos);
        }
        if (i != splitCnt - 1)
        {
            getIntersectionPaths(tempPaths, leftPath, tempPaths);
            SimplifyPolygons(paths, tempPaths);
            CleanPolygons(tempPaths, MINDISTANCE);
        }
    }
    distributionPathVec->push_back(leftPath);
    return (distInfoVec->size() == distributionPathVec->size());
}

void AlgorithmApplication::writeSupportAreaToPaths(QList<AREAINFOPTR> &listArea, Paths &supportPaths)
{
    writeAreaToPaths(listArea, supportPaths);
    if (BpcParas->nOptimize_1) scanLinesSortor->sortPaths(supportPaths);
}

void AlgorithmApplication::writeSupportToSList(const Paths &paths, const double &fSpeedRatio)
{
    if (_supportDisInfoVec.size() > 1)
    {
        std::vector<long long> posVec;
        ScanLinesDivider::calcScanLinesPos(paths, _supportDisInfoVec, posVec);

        int disIndex = 0;
        long long startPos = 0;
        long long pathCnt = 0;
        if (posVec.size() == _supportDisInfoVec.size())
        {
            auto totalPaths = std::move(paths);
            for (const auto &disInfo : qAsConst(_supportDisInfoVec))
            {
                pathCnt = posVec[disIndex] + 1 - startPos;
                Paths tempPaths(std::make_move_iterator(totalPaths.begin()),
                                std::make_move_iterator(totalPaths.begin() + pathCnt));
                totalPaths.erase(totalPaths.begin(), totalPaths.begin() + pathCnt);
                startPos = posVec[disIndex] + 1;
                ++ disIndex;

                UFFWRITEDATA mUFileData;
                writePathsToScanLines(tempPaths, mUFileData.listSLines);
                writeSupportToSList(mUFileData, fSpeedRatio, disInfo._containorIndex);
            }
        }
    }
    else
    {
        UFFWRITEDATA mUFileData;
        writePathsToScanLines(paths, mUFileData.listSLines);
        auto tempIndex = curScannerIndex;
        if(1 == _supportDisInfoVec.size()) tempIndex = _supportDisInfoVec.front()._containorIndex;
        writeSupportToSList(mUFileData, fSpeedRatio, tempIndex);
    }
}

void AlgorithmApplication::setBorderDistributionInfo(const std::vector<PartResult> &disInfoVec)
{
    _borderDisInfoVec = std::move(disInfoVec);
}

void AlgorithmApplication::setSupportDistributionInfo(const std::vector<PartResult> &disInfoVec)
{
    _supportDisInfoVec = std::move(disInfoVec);
}

void AlgorithmApplication::setSolidDistributionInfo(const std::vector<PartResult> &disInfoVec)
{
    _solidDisInfoVec = std::move(disInfoVec);
}

void AlgorithmApplication::resetDistributionInfo()
{
    _solidDisInfoVec.clear();
    _upsurfaceDisInfoVec.clear();
    _downsurfaceDisInfoVec.clear();

    _distributionPathVec.clear();
    _distributionUpsurfaceVec.clear();
    _distributionDownsurfaceVec.clear();
}

void AlgorithmApplication::setUpsurfaceDistributionInfo(const std::vector<PartResult> &disInfoVec)
{
    _upsurfaceDisInfoVec = std::move(disInfoVec);
}

void AlgorithmApplication::setDownsurfaceDistributionInfo(const std::vector<PartResult> &disInfoVec)
{
    _downsurfaceDisInfoVec = std::move(disInfoVec);
}

bool AlgorithmApplication::isBigBeam(const int &BeamCnt, const int &BeamIndex)
{
    if(1 == BeamCnt) return false;
    //    else if(0 == BeamIndex) return false;
    if(BeamCnt - 1 > BeamIndex) return false;
    return true;
}

void AlgorithmApplication::writePathsToScanLines(const Paths &paths, QVector<SCANLINE> &scanLines, const bool &closedPath)
{
    IntPoint tempPt(-1, -1);
    for(const auto &path : paths)
    {
        if(path.size() < 2) continue;

        size_t ptCnt = path.size();
        size_t i = 1;
        auto pt = path[0];
        if(tempPt != pt) {
            scanner_jumpPos(pt.X, pt.Y, scanLines);
            tempPt = pt;
        }
        while(i < ptCnt)
        {
            pt = path[i];
            if(tempPt != pt) {
                scanner_outLine(pt.X, pt.Y, scanLines);
                tempPt = pt;
            }
            i ++;
        }
        if(closedPath && path.size() > 2) {
            pt = path[0];
            if(tempPt != pt) {
                scanner_outLine(pt.X, pt.Y, scanLines);
            }
        }
    }
}

void AlgorithmApplication::writePathsToScanLines(const Path &path, QVector<SCANLINE> &scanLines, const bool &closedPath)
{
    if(path.size() < 2) return;

    IntPoint tempPt(-1, -1);
    size_t ptCnt = path.size();
    size_t i = 1;
    auto pt = path[0];
    if(tempPt != pt) {
        scanner_jumpPos(pt.X, pt.Y, scanLines);
        tempPt = pt;
    }
    while(i < ptCnt)
    {
        pt = path[i];
        if(tempPt != pt) {
            scanner_outLine(pt.X, pt.Y, scanLines);
            tempPt = pt;
        }
        i ++;
    }
    if(closedPath && path.size() > 2) {
        pt = path[0];
        if(tempPt != pt) {
            scanner_outLine(pt.X, pt.Y, scanLines);
        }
    }
}
