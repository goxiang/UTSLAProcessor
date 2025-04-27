#include "slmsplicingmodule.h"

#include "splicingmodulepriv.h"
#include "splicingabstract.h"
#include "splicingheader.h"
#include "algorithmbase.h"
#include "publicheader.h"

// #include "clipperinterface.h"
#include "clipper2/clipper.rectclip.h"

SLMSplicingModule::SLMSplicingModule() :
    d(new SLMSplicingModulePriv)
{
}

void SLMSplicingModule::initializeParas(const WriterBufferParas *writeBuff)
{
    auto bpcParas = writeBuff->_bpcParaPtr.data();

    AlgorithmBase algorith;
    QMap<int, ScanFieldInfo> &scanFiledMap = d->_scannerSplicing->scanFiledMap;
    QVector<CoincideFiledInfo> &coincideFieldVec = d->_scannerSplicing->coincideFieldVec;
    auto &splicingPtrVec = d->_scannerSplicing->splicingPtrVec;

    scanFiledMap.clear();
    coincideFieldVec.clear();

    const auto keys = bpcParas->rcScanFieldMap.keys();
    for(const auto &key : keys)
    {
        auto tempScanFileInfo = ScanFieldInfo();
        tempScanFileInfo.scanField << IntPoint(bpcParas->rcScanFieldMap[key].left() * UNITSPRECISION, bpcParas->rcScanFieldMap[key].top() * UNITSPRECISION)
                                   << IntPoint(bpcParas->rcScanFieldMap[key].right() * UNITSPRECISION, bpcParas->rcScanFieldMap[key].top() * UNITSPRECISION)
                                   << IntPoint(bpcParas->rcScanFieldMap[key].right() * UNITSPRECISION, bpcParas->rcScanFieldMap[key].bottom() * UNITSPRECISION)
                                   << IntPoint(bpcParas->rcScanFieldMap[key].left() * UNITSPRECISION, bpcParas->rcScanFieldMap[key].bottom() * UNITSPRECISION);
        tempScanFileInfo.scanRc = IntRect::GetBounds(tempScanFileInfo.scanField);
        scanFiledMap.insert(key, tempScanFileInfo);
    }

    int nScannerCnt = keys.size();
    auto insertSplicingInfo = [&splicingPtrVec, &writeBuff](const IntRect &subRc, ScanFieldInfo &scanField1, ScanFieldInfo &scanField2) {
        auto &scannerRc1 = scanField1.scanRc;
        auto &scannerRc2 = scanField2.scanRc;

        auto func_createSplicingLine = [&writeBuff](const int &lineType, const IntRect &subRc) -> SplicingAbstract * {
            auto splicingMode = writeBuff->getExtendedValue<int>("Splicing/nSplicingMode", 0);
            switch(splicingMode)
            {
            case 0:
            default:
            {
                auto splicingLine = new RandomLineSplicing();
                if(SplicingAbstract::SLT_Ver == lineType)
                {
                    splicingLine->initSplicingRanges(SplicingAbstract::SLT_Ver, subRc.left, subRc.right,
                                                     writeBuff->getExtendedValue<double>("SplicingRandomLine/fOverlap", 1),
                                                     writeBuff->getExtendedValue<double>("SplicingRandomLine/fSplicingRange", 50),
                                                     writeBuff->getExtendedValue<double>("SplicingRandomLine/fMinOffset", 5));
                }
                else
                {
                    splicingLine->initSplicingRanges(SplicingAbstract::SLT_Hor, subRc.top, subRc.bottom,
                                                     writeBuff->getExtendedValue<double>("SplicingRandomLine/fOverlap", 1),
                                                     writeBuff->getExtendedValue<double>("SplicingRandomLine/fSplicingRange", 50),
                                                     writeBuff->getExtendedValue<double>("SplicingRandomLine/fMinOffset", 5));
                }
                qDebug() << "RandomLineSplicing" << writeBuff->getExtendedValue<double>("SplicingRandomLine/fOverlap", 1)
                         << writeBuff->getExtendedValue<double>("SplicingRandomLine/fSplicingRange", 50)
                         << writeBuff->getExtendedValue<double>("SplicingRandomLine/fMinOffset", 5);
                return splicingLine;
            }
            case 1:
            {
                auto splicingLine = new PeriodicLine();
                if(SplicingAbstract::SLT_Ver == lineType)
                {
                    splicingLine->initSplicingRanges(SplicingAbstract::SLT_Ver, subRc.left, subRc.right,
                                                     writeBuff->getExtendedValue<double>("PeriodicLine/fOverlap", 1),
                                                     writeBuff->getExtendedValue<double>("PeriodicLine/fSplicingRange", 50),
                                                     writeBuff->getExtendedValue<int>("PeriodicLine/nRepeatCnt", 5),
                                                     writeBuff->getExtendedValue<double>("PeriodicLine/fInitialPos", 5),
                                                     writeBuff->getExtendedValue<double>("PeriodicLine/fStepLength", 5));
                }
                else
                {
                    splicingLine->initSplicingRanges(SplicingAbstract::SLT_Hor, subRc.top, subRc.bottom,
                                                     writeBuff->getExtendedValue<double>("PeriodicLine/fOverlap", 1),
                                                     writeBuff->getExtendedValue<double>("PeriodicLine/fSplicingRange", 50),
                                                     writeBuff->getExtendedValue<int>("PeriodicLine/nRepeatCnt", 5),
                                                     writeBuff->getExtendedValue<double>("PeriodicLine/fInitialPos", 5),
                                                     writeBuff->getExtendedValue<double>("PeriodicLine/fStepLength", 5));
                }
                qDebug() << "PeriodicLine" <<
                    writeBuff->getExtendedValue<double>("PeriodicLine/fOverlap", 1) <<
                    writeBuff->getExtendedValue<double>("PeriodicLine/fSplicingRange", 50) <<
                    writeBuff->getExtendedValue<int>("PeriodicLine/nRepeatCnt", 5) <<
                    writeBuff->getExtendedValue<double>("PeriodicLine/fInitialPos", 5) <<
                    writeBuff->getExtendedValue<double>("PeriodicLine/fStepLength", 5);
                return splicingLine;
            }
            }

            return nullptr;
        };

        if((subRc.left == scannerRc1.left && subRc.right == scannerRc2.right)
            && (scannerRc1.top == scannerRc2.top)
            && !scanField1.splicingMap.contains(SplicingAbstract::RT_Left)
            && !scanField2.splicingMap.contains(SplicingAbstract::RT_Right))
        {
            bool existSplicingLine = false;
            for(const auto &splicingLine : splicingPtrVec)
            {
                const SplicingBaseSettings &splicingSettings = splicingLine->getSplicingSettings();
                if(SplicingAbstract::SLT_Ver == splicingSettings.splicingLineType) {
                    if(splicingSettings.minRangePos == subRc.left && splicingSettings.maxRangePos == subRc.right) {
                        existSplicingLine = true;
                        scanField1.splicingMap.insert(SplicingAbstract::RT_Left, splicingLine);
                        scanField2.splicingMap.insert(SplicingAbstract::RT_Right, splicingLine);
                        break;
                    }
                }
            }
            if(false == existSplicingLine)
            {
                if(auto splicingLine = QSharedPointer<SplicingAbstract>(func_createSplicingLine(SplicingAbstract::SLT_Ver, subRc)))
                {
                    splicingPtrVec << splicingLine;
                    scanField1.splicingMap.insert(SplicingAbstract::RT_Left, splicingLine);
                    scanField2.splicingMap.insert(SplicingAbstract::RT_Right, splicingLine);
                }
            }
        }
        else if((subRc.left == scannerRc2.left && subRc.right == scannerRc1.right)
                 && (scannerRc1.top == scannerRc2.top)
                 && !scanField1.splicingMap.contains(SplicingAbstract::RT_Right)
                 && !scanField2.splicingMap.contains(SplicingAbstract::RT_Left))
        {
            bool existSplicingLine = false;
            for(const auto &splicingLine : splicingPtrVec)
            {
                const SplicingBaseSettings &splicingSettings = splicingLine->getSplicingSettings();
                if(SplicingAbstract::SLT_Ver == splicingSettings.splicingLineType) {
                    if(splicingSettings.minRangePos == subRc.left && splicingSettings.maxRangePos == subRc.right) {
                        existSplicingLine = true;
                        scanField1.splicingMap.insert(SplicingAbstract::RT_Right, splicingLine);
                        scanField2.splicingMap.insert(SplicingAbstract::RT_Left, splicingLine);
                        break;
                    }
                }
            }
            if(false == existSplicingLine)
            {
                if(auto splicingLine = QSharedPointer<SplicingAbstract>(func_createSplicingLine(SplicingAbstract::SLT_Ver, subRc)))
                {
                    splicingPtrVec << splicingLine;
                    scanField1.splicingMap.insert(SplicingAbstract::RT_Right, splicingLine);
                    scanField2.splicingMap.insert(SplicingAbstract::RT_Left, splicingLine);
                }
            }
        }
        else if((subRc.bottom == scannerRc1.bottom && subRc.top == scannerRc2.top)
                 && (scannerRc1.right == scannerRc2.right)
                 && !scanField1.splicingMap.contains(SplicingAbstract::RT_Bottom)
                 && !scanField2.splicingMap.contains(SplicingAbstract::RT_Top))
        {
            bool existSplicingLine = false;
            for(const auto &splicingLine : splicingPtrVec)
            {
                const SplicingBaseSettings &splicingSettings = splicingLine->getSplicingSettings();
                if(SplicingAbstract::SLT_Hor == splicingSettings.splicingLineType) {
                    if(splicingSettings.minRangePos == subRc.top && splicingSettings.maxRangePos == subRc.bottom) {
                        existSplicingLine = true;
                        scanField1.splicingMap.insert(SplicingAbstract::RT_Bottom, splicingLine);
                        scanField2.splicingMap.insert(SplicingAbstract::RT_Top, splicingLine);
                        break;
                    }
                }
            }
            if(false == existSplicingLine)
            {
                if(auto splicingLine = QSharedPointer<SplicingAbstract>(func_createSplicingLine(SplicingAbstract::SLT_Hor, subRc)))
                {
                    splicingPtrVec << splicingLine;
                    scanField1.splicingMap.insert(SplicingAbstract::RT_Bottom, splicingLine);
                    scanField2.splicingMap.insert(SplicingAbstract::RT_Top, splicingLine);
                }
            }
        }
        else if((subRc.bottom == scannerRc2.bottom && subRc.top == scannerRc1.top)
                 && (scannerRc1.right == scannerRc2.right)
                 && !scanField1.splicingMap.contains(SplicingAbstract::RT_Top)
                 && !scanField2.splicingMap.contains(SplicingAbstract::RT_Bottom))
        {
            bool existSplicingLine = false;
            for(const auto &splicingLine : splicingPtrVec)
            {
                const SplicingBaseSettings &splicingSettings = splicingLine->getSplicingSettings();
                if(SplicingAbstract::SLT_Hor == splicingSettings.splicingLineType) {
                    if(splicingSettings.minRangePos == subRc.top && splicingSettings.maxRangePos == subRc.bottom) {
                        existSplicingLine = true;
                        scanField1.splicingMap.insert(SplicingAbstract::RT_Top, splicingLine);
                        scanField2.splicingMap.insert(SplicingAbstract::RT_Bottom, splicingLine);
                        break;
                    }
                }
            }
            if(false == existSplicingLine)
            {
                if(auto splicingLine = QSharedPointer<SplicingAbstract>(func_createSplicingLine(SplicingAbstract::SLT_Hor, subRc)))
                {
                    splicingPtrVec << splicingLine;
                    scanField1.splicingMap.insert(SplicingAbstract::RT_Top, splicingLine);
                    scanField2.splicingMap.insert(SplicingAbstract::RT_Bottom, splicingLine);
                }
            }
        }
    };

    for(int i = 0; i < nScannerCnt - 1; ++ i)
    {
        for(int j = i + 1; j < nScannerCnt; ++ j)
        {
            Paths coincidePath;
            algorith.getUnionPaths(scanFiledMap[i].scanField, scanFiledMap[j].scanField, coincidePath, 0);
            insertSplicingInfo(IntRect::GetBounds(coincidePath), scanFiledMap[i], scanFiledMap[j]);
        }
    }
    for(int i = 0; i < nScannerCnt - 1; ++ i)
    {
        for(int j = i + 1; j < nScannerCnt; ++ j)
        {
            Paths coincidePath;
            CoincideFiledInfo coincideFiledInfo;
            algorith.getUnionPaths(scanFiledMap[i].scanField, scanFiledMap[j].scanField, coincidePath, 0);
            if(Areas(coincidePath))
            {
                coincideFiledInfo.subPaths = coincidePath;
                coincideFiledInfo.scannerSet << i << j;

                auto tempCoincideVec = coincideFieldVec;
                coincideFieldVec.clear();
                for(auto &coincide : tempCoincideVec)
                {
                    {
                        CoincideFiledInfo diff_coincide;
                        algorith.getDifferencePaths(&coincide.subPaths, &coincidePath, diff_coincide.subPaths, 0);
                        if(Areas(diff_coincide.subPaths))
                        {
                            diff_coincide.scannerSet = coincide.scannerSet;
                            coincideFieldVec << diff_coincide;
                        }
                    }
                    {
                        CoincideFiledInfo intersection_coincide;
                        algorith.getUnionPaths(&coincide.subPaths, &coincidePath, intersection_coincide.subPaths, 0);
                        if(Areas(intersection_coincide.subPaths))
                        {
                            intersection_coincide.scannerSet = coincide.scannerSet + (QSet<int>() << i << j);
                            coincideFieldVec << intersection_coincide;
                        }
                    }

                    algorith.getDifferencePaths(&coincideFiledInfo.subPaths, &coincide.subPaths, coincideFiledInfo.subPaths, 0);
                }

                if(Areas(coincideFiledInfo.subPaths))
                {
                    coincideFieldVec << coincideFiledInfo;
                }
            }
        }
    }

    for(int i = 0 ; i < nScannerCnt; ++ i)
    {
        Paths independentPaths;
        independentPaths << scanFiledMap[i].scanField;
        for(int j = 0; j < nScannerCnt; ++ j)
        {
            if(i != j)
            {
                algorith.getDifferencePaths(independentPaths, scanFiledMap[j].scanField, independentPaths, 0);
            }
        }
        scanFiledMap[i].independentPaths = independentPaths;
    }
}

void SLMSplicingModule::recalcSplicingArea(Paths &solidPaths, QList<AREAINFOPTR> &supportPaths, const QList<AREAINFOPTR> &solidSupports)
{
//    auto splicingMode = d->jsonParsing.getValue<int>("/Splicing/SplicingMode");
//    if(0 == splicingMode)
//    {
//        SplicingFixedMode::recalcSplicingArea(solidPaths, supportPaths, solidSupports, d);
//    }
//    d->splicingAreaMap_support.clear();
//    auto keys = d->scanFiledMap.keys();
//    auto splicingArea =  new SplicingAreaInfo();
//    splicingArea->scannerIndex = 1;
//    splicingArea->layerScanField << (Path() << IntPoint(0, 0) << IntPoint(305000, 0) << IntPoint(305000, 215000) <<  IntPoint(0, 215000));
//    for(const auto &key : keys)
//    {
//        auto splicingArea =  new SplicingAreaInfo();
//        splicingArea->scannerIndex = key;
//        splicingArea->layerScanField << (Path() << IntPoint(0, 0) << IntPoint(305000, 0) << IntPoint(305000, 215000) <<  IntPoint(0, 215000));
//        d->splicingAreaMap_support.insert(key, splicingArea);
//    }

    LayerSplicingArea &layerSplicing = d->_scannerSplicing->layerSplicingArea;
    for(auto &splicingPtr : qAsConst(d->_scannerSplicing->splicingPtrVec))
    {
        splicingPtr->calcNextPos();
    }

    layerSplicing.splicingAreaMap_support.clear();

    const auto keys = d->_scannerSplicing->scanFiledMap.keys();
    for(const auto &key : keys)
    {
        const auto &scanFiled = d->_scannerSplicing->scanFiledMap[key];
        const auto splicingKeys = scanFiled.splicingMap.keys();

        auto splicingArea =  new SplicingAreaInfo();

        if(splicingKeys.contains(SplicingAbstract::RT_Left))
        {
            const auto &splicingLine = scanFiled.splicingMap[SplicingAbstract::RT_Left];
            splicingArea->rectField.left = splicingLine->getSplicingPos() - splicingLine->getOverlap();
        }
        else splicingArea->rectField.left = scanFiled.scanRc.left;

        if(splicingKeys.contains(SplicingAbstract::RT_Right))
        {
            const auto &splicingLine = scanFiled.splicingMap[SplicingAbstract::RT_Right];
            splicingArea->rectField.right = splicingLine->getSplicingPos() + splicingLine->getOverlap();
        }
        else splicingArea->rectField.right = scanFiled.scanRc.right;

        if(splicingKeys.contains(SplicingAbstract::RT_Top))
        {
            const auto &splicingLine = scanFiled.splicingMap[SplicingAbstract::RT_Top];
            splicingArea->rectField.top = splicingLine->getSplicingPos() - splicingLine->getOverlap();
        }
        else splicingArea->rectField.top = scanFiled.scanRc.top;

        if(splicingKeys.contains(SplicingAbstract::RT_Bottom))
        {
            const auto &splicingLine = scanFiled.splicingMap[SplicingAbstract::RT_Bottom];
            splicingArea->rectField.bottom = splicingLine->getSplicingPos() + splicingLine->getOverlap();
        }
        else splicingArea->rectField.bottom = scanFiled.scanRc.bottom;

        splicingArea->scannerIndex = key;
        splicingArea->layerScanField.clear();
        splicingArea->layerScanField << splicingArea->rectField.AsPath();
        layerSplicing.splicingAreaMap_support.insert(key, SplicingAreaPtr(splicingArea));
    }
}

void SLMSplicingModule::addLayerSplicingArea(const int &layerHei)
{
    LayerSplicingArea layerSplicing;
    for(auto &splicingPtr : qAsConst(d->_scannerSplicing->splicingPtrVec))
    {
        splicingPtr->calcNextPos();
    }

    layerSplicing.splicingAreaMap_support.clear();
    const auto keys = d->_scannerSplicing->scanFiledMap.keys();
    for(const auto &key : keys)
    {
        const auto &scanFiled = d->_scannerSplicing->scanFiledMap[key];
        const auto splicingKeys = scanFiled.splicingMap.keys();

        auto splicingArea =  new SplicingAreaInfo();

        if(splicingKeys.contains(SplicingAbstract::RT_Left))
        {
            const auto &splicingLine = scanFiled.splicingMap[SplicingAbstract::RT_Left];
            splicingArea->rectField.left = splicingLine->getSplicingPos() - splicingLine->getOverlap();
        }
        else splicingArea->rectField.left = scanFiled.scanRc.left;

        if(splicingKeys.contains(SplicingAbstract::RT_Right))
        {
            const auto &splicingLine = scanFiled.splicingMap[SplicingAbstract::RT_Right];
            splicingArea->rectField.right = splicingLine->getSplicingPos() + splicingLine->getOverlap();
        }
        else splicingArea->rectField.right = scanFiled.scanRc.right;

        if(splicingKeys.contains(SplicingAbstract::RT_Top))
        {
            const auto &splicingLine = scanFiled.splicingMap[SplicingAbstract::RT_Top];
            splicingArea->rectField.top = splicingLine->getSplicingPos() - splicingLine->getOverlap();
        }
        else splicingArea->rectField.top = scanFiled.scanRc.top;

        if(splicingKeys.contains(SplicingAbstract::RT_Bottom))
        {
            const auto &splicingLine = scanFiled.splicingMap[SplicingAbstract::RT_Bottom];
            splicingArea->rectField.bottom = splicingLine->getSplicingPos() + splicingLine->getOverlap();
        }
        else splicingArea->rectField.bottom = scanFiled.scanRc.bottom;

        splicingArea->scannerIndex = key;
        splicingArea->layerScanField.clear();
        splicingArea->layerScanField.push_back(splicingArea->rectField.AsPath());
        layerSplicing.splicingAreaMap_support.insert(key, SplicingAreaPtr(splicingArea));
    }
    d->_layerSplicingMap.insert(layerHei, layerSplicing);
}

inline QSharedPointer<ScannerSplicingArea> SLMSplicingModule::getSplicingArea() const
{
    return d->_scannerSplicing;
}

LayerSplicingArea SLMSplicingModule::getLayerSplicing(const int &layerHei) const
{
    if(d->_layerSplicingMap.contains(layerHei)) return d->_layerSplicingMap[layerHei];
    return LayerSplicingArea();
}

void SLMSplicingModule::splitSupportData(const LayerSplicingArea &layerSplicing, const Paths &supportPahts, FuncSplit &&funcSplit) const
{
    const auto keys = layerSplicing.splicingAreaMap_support.keys();
    for(const auto &key : keys)
    {
        const Clipper2Lib::Paths64 paths = *(reinterpret_cast<const Clipper2Lib::Paths64 *>(&supportPahts));
        Clipper2Lib::Rect<int64_t> rect(layerSplicing.splicingAreaMap_support[key]->rectField.left,
                                        layerSplicing.splicingAreaMap_support[key]->rectField.top,
                                        layerSplicing.splicingAreaMap_support[key]->rectField.right,
                                        layerSplicing.splicingAreaMap_support[key]->rectField.bottom);
        Clipper2Lib::RectClipLines64 rectClipper(rect);
        auto tempSol = rectClipper.Execute(paths);
        Paths sol_1 = *(reinterpret_cast<Paths *>(&tempSol));
        if(sol_1.size()) funcSplit(key, sol_1);
    }
}

void SLMSplicingModule::splitBorderData(const LayerSplicingArea &layerSplicing, const Paths &paths,
                                        FuncSplit &&funcSplit, const bool &closeBorder,
                                        const std::function<void(Paths &)> &&newBegin) const
{
    AlgorithmBase algorith;
    auto keys = layerSplicing.splicingAreaMap_support.keys();
    for(const auto &key : keys)
    {
        auto &splicingArea = layerSplicing.splicingAreaMap_support[key];

        Paths sol;
        if (closeBorder)
        {
            algorith.getUnionPaths(&paths, &splicingArea->layerScanField, sol, 0);
            // intersectPaths(*(reinterpret_cast<const Clipper2Lib::Paths64 *>(&paths)),
            //                *(reinterpret_cast<const Clipper2Lib::Paths64 *>(&splicingArea->layerScanField)),
            //                *(reinterpret_cast<Clipper2Lib::Paths64 *>(&sol)));
            newBegin(sol);
        }
        else
        {
            const IntRect &rect = splicingArea->rectField;
            Paths onRectLineVec;
            d->calcLinesOnRect(rect, paths, onRectLineVec);

            algorith.getUnionPaths(&paths, &splicingArea->layerScanField, sol, 0);
            // intersectPaths(*(reinterpret_cast<const Clipper2Lib::Paths64 *>(&paths)),
            //                *(reinterpret_cast<const Clipper2Lib::Paths64 *>(&splicingArea->layerScanField)),
            //                *(reinterpret_cast<Clipper2Lib::Paths64 *>(&sol)));
            newBegin(sol);
            d->removeLineOnRect(rect, sol);
            sol << onRectLineVec;
        }

        if(sol.size()) funcSplit(key, sol);
    }
}

void SLMSplicingModule::splitSolidData(const LayerSplicingArea &layerSplicing, const Paths &paths, FuncSplit &&funcSplit) const
{
    AlgorithmBase algorith;
    auto keys = layerSplicing.splicingAreaMap_support.keys();
    for(const auto &key : keys)
    {
        auto &splicingArea = layerSplicing.splicingAreaMap_support[key];
        Paths sol;
        algorith.getUnionPaths(&paths, &splicingArea->layerScanField, sol, 0);
        if(sol.size()) funcSplit(key, sol);
    }
}
