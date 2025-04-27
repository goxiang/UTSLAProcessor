#include "dividerprocessor.h"

#include "utslaprocessorprivate.h"
#include "algorithmdistribution.h"
#include "bppbuildparameters.h"
#include "utslaprocessor.h"
#include "uspfilewriter.h"
#include "publicheader.h"

#include <algorithm>
#include <memory>
#include <list>

#include "SelfAdaptiveModule/selfadaptivemodule.h"
#include "./PolygonsDivider/polygonsdivider.h"
#include "./DynamicDivider/publicheader.h"
#include "qlog.h"

#include <QtConcurrent>
#include <QDebug>


#define Calc_Support
#define Calc_Border
#define Calc_Solid

class MemGuard {
public:
    MemGuard(std::function<void()> &&func) { _func = std::move(func); }
    ~MemGuard() { _func(); }

private:
    std::function<void()> _func = [](){};
};

struct BuildPart {
    int _border_scanner_index = 0;
    SOLIDPATH _solidPath;
    USPFileWriterPtr _fileWriterPtr = nullptr;
    int _nLastLayer = -1;
    double _fLayerThickness = 0.1;
};

DividerProcessor::DividerProcessor(UTSLAProcessorPrivate *slaPriv)
{
    _runningSemaphore.release();
    _slaPriv = slaPriv;
}

void DividerProcessor::startProcessing()
{
    MemGuard guard([this]() { _runningSemaphore.release(); });
    _runningSemaphore.acquire();

    // qDebug() << "startProcessing" << partCnt << curLayer << endLayer;
    createPartWriter();
    processingAllParts();
    writeFileEnd();
}

void DividerProcessor::createPartWriter()
{
    auto q = _slaPriv->q_ptr;
    if (false == q->isRunning()) return;

    auto scannerCnt = _slaPriv->_writerBufferParas->getExtendedValue<int>("Splicing/nNumber_SplicingScanner", 1);

    auto allowed_border = _slaPriv->_writerBufferParas->getExtendedValue<QStringList>("Splicing/allowedScanners_border", {"0"});
    auto allowed_support = _slaPriv->_writerBufferParas->getExtendedValue<QStringList>("Splicing/allowedScanners_support", {"0"});
    auto allowed_hatching = _slaPriv->_writerBufferParas->getExtendedValue<QStringList>("Splicing/allowedScanners_hatching", {"0"});
    auto allowed_upface = _slaPriv->_writerBufferParas->getExtendedValue<QStringList>("Splicing/allowedScanners_upface", {"0"});
    auto allowed_downface = _slaPriv->_writerBufferParas->getExtendedValue<QStringList>("Splicing/allowedScanners_downface", {"0"});

    auto funcGetAllowedSet = [](const int &totalCnt, const QStringList &allowdScanners) -> std::set<int> {
        std::set<int> allowedSet;
        for (QString str : allowdScanners) {
            auto index = str.replace(" ", "").toInt();
            if (index > -1 && index < totalCnt) allowedSet.insert(index);
        }
        if (allowedSet.size() < 1) allowedSet.insert(0);
        return allowedSet;
    };

    auto funcGetAllowedLVec = [](const int &totalCnt, const QStringList &allowdScanners) -> std::vector<int> {
        std::vector<int> allowedList;
        for (QString str : allowdScanners) {
            auto index = str.replace(" ", "").toInt();
            if (index > -1 && index <= totalCnt &&
                std::find(allowedList.begin(), allowedList.end(), index) == allowedList.end()) {
                allowedList.push_back(index);
            }
        }
        if (allowedList.size() < 1) allowedList.push_back(0);
        return allowedList;
    };

    _allowedSet_support = funcGetAllowedSet(scannerCnt, allowed_support);
    _allowedSet_hatching = funcGetAllowedSet(scannerCnt, allowed_hatching);
    _allowedSet_upface = funcGetAllowedSet(scannerCnt, allowed_upface);
    _allowedSet_downface = funcGetAllowedSet(scannerCnt, allowed_downface);

    std::vector<int> allowedList_border = funcGetAllowedLVec(scannerCnt, allowed_border);
    int fixedScanner_border = -1;
    if (1 == allowedList_border.size()) fixedScanner_border = allowedList_border.front();

    _partCnt = q->getBuildPartCount();
    _minLayer = _CRT_INT_MAX;
    _maxLayer = -1;
    std::list<std::pair<QSharedPointer<BuildPart>, int>> buildPartList;
    for(int partIndex = 0; partIndex < _partCnt; ++ partIndex)
    {
        if (false == q->isRunning()) break;
        _minLayer = qMin(_minLayer, q->getMinLayer(partIndex));
        _maxLayer = qMax(_maxLayer, q->getMaxLayer(partIndex));

        auto buildPart = QSharedPointer<BuildPart>(new BuildPart);
        buildPart->_nLastLayer = _minLayer - 1;
        _slaPriv->createFileWriter(partIndex, buildPart->_fileWriterPtr);

        if (-1 != fixedScanner_border)
        {
            buildPart->_border_scanner_index = fixedScanner_border;
            std::vector<PartResult> partResultVec;
            partResultVec.push_back(PartResult(buildPart->_border_scanner_index, 1));
            buildPart->_fileWriterPtr->algo()->setBorderDistributionInfo(partResultVec);
        }

        _buildPartMap.insert(partIndex, buildPart);
        _divideDataMap.insert(partIndex, QSharedPointer<DivideSolidData>(new DivideSolidData()));
        buildPartList.push_back({buildPart, partIndex});
    }

    if (-1 == fixedScanner_border)
    {
        int allowedSize = allowedList_border.size();
        buildPartList.sort([&q](const std::pair<QSharedPointer<BuildPart>, int> &part1, const std::pair<QSharedPointer<BuildPart>, int> &part2) -> bool {
            return q->getMaxLayer(part1.second) < q->getMaxLayer(part2.second);
        });

        int tempIndex = 0;
        for (const auto &partInfo : buildPartList)
        {
            auto buildPart = partInfo.first;
            buildPart->_border_scanner_index = allowedList_border[(tempIndex ++) % allowedSize];
            std::vector<PartResult> partResultVec;
            partResultVec.push_back(PartResult(buildPart->_border_scanner_index, 1));
            buildPart->_fileWriterPtr->algo()->setBorderDistributionInfo(partResultVec);
        }
    }
}

void DividerProcessor::processingAllParts()
{
    auto q = _slaPriv->q_ptr;
    if (false == q->isRunning()) return;

    auto curLayer = _minLayer;
    auto lastLayer = curLayer;

    // curLayer = 5500;
    // _maxLayer = 6500;
    double fStep = 100.0 / (_maxLayer - _minLayer - 1);

    QList<int> validSolidKeys;
    QMap<int, Paths> validSupportPartMap;
    std::vector<std::shared_ptr<DistPartInfo>> partInfoVec;

    QMutex areaLocker;
    QMutex partLocker;
    QMutex supportLocker;

    QVector<int> partVec;
    partVec.reserve(_partCnt);
    for (int i = 0; i < _partCnt; ++ i) partVec << i;

    auto readDataAndCalculateWeight = [&, this](const int &partIndex){
        if (q->isEffectiveLayer(curLayer, partIndex))
        {
            auto uspWriter = _buildPartMap[partIndex]->_fileWriterPtr;
            SOLIDPATH &solidPath = _buildPartMap[partIndex]->_solidPath;
            updatePartLayerInfo(partIndex, curLayer, _buildPartMap[partIndex]);
            BOUNDINGRECT totalRc;

            _slaPriv->readLayerDatas(curLayer, partIndex, uspWriter, solidPath);
            if (solidPath.lpPath_Cur->size())
            {
                SimplifyPolygons(*solidPath.lpPath_Cur);
                uspWriter->algo()->calcLimitXY(*solidPath.lpPath_Cur, totalRc);
                partLocker.lock();
                validSolidKeys.append(partIndex);
                partLocker.unlock();
            }

            QList<AREAINFOPTR> areaSupport;
            q->readLayerDatas(curLayer, areaSupport, partIndex, FTYPE_SUPPORT);
            if (areaSupport.size())
            {
                uspWriter->algo()->calcLimitXY(areaSupport, totalRc);

                Paths paths;
                uspWriter->algo()->writeSupportAreaToPaths(areaSupport, paths);
                supportLocker.lock();
                validSupportPartMap.insert(partIndex, paths);
                supportLocker.unlock();
#ifdef Calc_Support
                auto partPtr = std::shared_ptr<DistPartInfo>(new DistPartInfo(partIndex, calcSupportWeight(paths),
                                                                              DistAreaType::Support, _allowedSet_support));
                areaLocker.lock();
                partInfoVec.push_back(partPtr);
                areaLocker.unlock();
#endif
            }

            uspWriter->startBuffWriter();
            uspWriter->algo()->setCurLayerHei(curLayer);
            uspWriter->writeLayerInfo(curLayer, totalRc, 0);

            if (solidPath.lpPath_Cur->size())
            {
                // Paths tempPaths;
                // uspWriter->algo()->getOffsetPaths(solidPath.lpPath_Cur, &tempPaths, 800, 0, 0);
                // double fArea = Areas(tempPaths);
                double fArea = Areas(*solidPath.lpPath_Cur);
                uspWriter->algo()->keepHolesAndGaps(solidPath.lpPath_Cur, solidPath.paths_smallHoles, solidPath.paths_smallGaps,
                                                    solidPath.paths_exceptHolesAndGaps);
#ifdef Calc_Solid
                auto divideData = _divideDataMap[partIndex].data();
                uspWriter->algo()->calcDivideData(solidPath, *divideData);

                if (divideData->_paths_Inner.size())
                {
                    auto partPtr = std::shared_ptr<DistPartInfo>(new DistPartInfo(partIndex, Areas(divideData->_paths_Inner),
                                                                                  DistAreaType::Hatching, _allowedSet_hatching));
                    areaLocker.lock();
                    partInfoVec.push_back(partPtr);
                    areaLocker.unlock();
                }
                if (divideData->_paths_up.size())
                {
                    auto partPtr = std::shared_ptr<DistPartInfo>(new DistPartInfo(partIndex,
                                                                                  calcUpfaceWeight(divideData->_paths_up, divideData->_nIndex_up),
                                                                                  DistAreaType::Upface, _allowedSet_upface));
                    areaLocker.lock();
                    partInfoVec.push_back(partPtr);
                    areaLocker.unlock();
                }
                if (divideData->_paths_down.size())
                {
                    auto partPtr = std::shared_ptr<DistPartInfo>(new DistPartInfo(partIndex,
                                                                                  calcDownfaceWeight(divideData->_paths_down, divideData->_nIndex_down),
                                                                                  DistAreaType::Downface, _allowedSet_downface));
                    areaLocker.lock();
                    partInfoVec.push_back(partPtr);
                    areaLocker.unlock();
                }
#endif
                fArea *= AREAFACTOR;
                uspWriter->addVolume(fArea * _buildPartMap[partIndex]->_fLayerThickness);
                uspWriter->writeAreaInfo(int(fArea));
            }
        }
    };

    while (curLayer <= _maxLayer)
    {
        if (false == q->isRunning()) break;

        partInfoVec.clear();
        validSolidKeys.clear();
        validSupportPartMap.clear();

        // QFutureWatcher<void> futureWatcher;
        // futureWatcher.setFuture(QtConcurrent::map(partVec, readDataAndCalculateWeight));
        // futureWatcher.waitForFinished();

#pragma omp parallel for
        for (int i = 0; i < _partCnt; ++ i)
        {
            readDataAndCalculateWeight(i);
        }

        if (false == validSolidKeys.empty() || false == validSupportPartMap.empty())
        {
            const auto supportKeys = validSupportPartMap.keys();
            const auto totalKeys = supportKeys.toSet() + validSolidKeys.toSet();

#ifdef Calc_Border
            if (validSolidKeys.size()) calcBorderWeight(validSolidKeys, partInfoVec);
#endif

            AlgorithmDistribution::distribution(partInfoVec);
            for (const auto &partPtr : partInfoVec)
            {
                auto p_index = partPtr->_index;
                switch (partPtr->_distAreaType) {
                case DistAreaType::Border:
                    break;
                case DistAreaType::Support:
                    _buildPartMap[p_index]->_fileWriterPtr->algo()->setSupportDistributionInfo(partPtr->_distResultVec);
                    break;
                case DistAreaType::Hatching:
                    _buildPartMap[p_index]->_fileWriterPtr->algo()->setSolidDistributionInfo(partPtr->_distResultVec);
                    break;
                case DistAreaType::Upface:
                    _buildPartMap[p_index]->_fileWriterPtr->algo()->setUpsurfaceDistributionInfo(partPtr->_distResultVec);
                    break;
                case DistAreaType::Downface:
                    _buildPartMap[p_index]->_fileWriterPtr->algo()->setDownsurfaceDistributionInfo(partPtr->_distResultVec);
                    break;
                }
            }
            runTaskInConcurrent(totalKeys, supportKeys, validSolidKeys, validSupportPartMap);

            _slaPriv->updateProgress(double(curLayer - lastLayer) * fStep);
            lastLayer = curLayer;
        }
        ++ curLayer;
    }
}

void DividerProcessor::writeFileEnd()
{
    auto q = _slaPriv->q_ptr;
    if (false == q->isRunning()) return;

    const auto keys = _buildPartMap.keys();
    for (const auto index : keys)
    {
        auto uspWriter = _buildPartMap[index]->_fileWriterPtr;
        uspWriter->writeFileEnd();

        if(q->isRunning())
        {
            QJsonObject jsonPartInfo;
            uspWriter->createUSPFile(q->getBuildPartName(index), _slaPriv->strBPPName,
                                     q->getLayerThickness(index), q->getBoundingBox(index),
                                     q->getMinLayer(index), q->getMaxLayer(index), jsonPartInfo);
            _slaPriv->objZipDescFile << jsonPartInfo;
        }
    }
}

void DividerProcessor::updatePartLayerInfo(const int &index, const int &curLayer,
                                           QSharedPointer<BuildPart> &buildPartInfo)
{
    auto q = _slaPriv->q_ptr;
    auto layerThickness = curLayer - buildPartInfo->_nLastLayer;
    if(1 == layerThickness)
    {
        buildPartInfo->_fLayerThickness = double(q->getLayerThickness(index));
    }
    else
    {
        buildPartInfo->_fLayerThickness = layerThickness * LAYERHEIFACTOR;
    }
    buildPartInfo->_nLastLayer = curLayer;
}

struct LastPtStatus {
    bool _begining = true;
    long long _lastX = 0;
    long long _lastY = 0;

    long long _markCnt = 0;
    long long _jumpCnt = 0;
};

void calcBorderPathsWeight(const Paths &paths, double &weight_jump, double &weight_mark,
                           double &jumpCnt, double &martCnt, LastPtStatus &lastPtStatus)
{
    lastPtStatus._jumpCnt += paths.size();
    for (const auto &path : paths)
    {
        if (path.size() < 2) continue;
        lastPtStatus._markCnt += path.size();
        if (lastPtStatus._begining) lastPtStatus._begining = false;
        else
        {
            jumpCnt += 1;
            weight_jump += sqrt(pow(lastPtStatus._lastX - path.front().X, 2) +
                                pow(lastPtStatus._lastY - path.front().Y, 2));
        }
        lastPtStatus._lastX = path.back().X;
        lastPtStatus._lastY = path.back().Y;

        int index = path.size() - 1;
        for (auto nextIndex = 0; nextIndex < path.size(); ++ nextIndex)
        {
            martCnt += 1;
            weight_mark += sqrt(pow(path[nextIndex].X - path[index].X, 2) +
                                pow(path[nextIndex].Y - path[index].Y, 2));
            index = nextIndex;
        }
    }
}

void calcSupportPathsWeight(const Paths &paths, double &weight_jump, double &weight_mark,
                            double &jumpCnt, double &martCnt, LastPtStatus &lastPtStatus)
{
    lastPtStatus._jumpCnt += paths.size();
    for (const auto &path : paths)
    {
        if (path.size() < 2) continue;
        lastPtStatus._markCnt += path.size();

        if (lastPtStatus._begining) lastPtStatus._begining = false;
        else
        {
            jumpCnt += 1;
            weight_jump += sqrt(pow(lastPtStatus._lastX - path.front().X, 2) +
                                pow(lastPtStatus._lastY - path.front().Y, 2));
        }
        lastPtStatus._lastX = path.back().X;
        lastPtStatus._lastY = path.back().Y;

        int index = 0;
        for (auto nextIndex = 1; nextIndex < path.size(); ++ nextIndex)
        {
            martCnt += 1;
            weight_mark += sqrt(pow(path[nextIndex].X - path[index].X, 2) +
                                pow(path[nextIndex].Y - path[index].Y, 2));
            index = nextIndex;
        }
    }
}

void DividerProcessor::calcBorderWeight(const QList<int> &validList, std::vector<std::shared_ptr<DistPartInfo>> &partInfoVec)
{
    QMap<int, QList<int>> allowedScanners_borderet;
    for (const auto &index : validList)
    {
        allowedScanners_borderet[_buildPartMap[index]->_border_scanner_index] << index;
    }

    const auto scanners = allowedScanners_borderet.keys();
    for (const auto &scanner : scanners)
    {
        auto partPtr = std::shared_ptr<DistPartInfo>(new DistPartInfo(-1 - scanner,
                                                                      calcBorderWeight(allowedScanners_borderet[scanner]),
                                                                      DistAreaType::Border, {scanner}));
        partInfoVec.push_back(partPtr);
    }
}

double DividerProcessor::calcBorderWeight(const QList<int> &validList)
{
    auto _writeBuff = _slaPriv->_writerBufferParas.data();
    if (_writeBuff->_bppParaPtr->sBorderPara.nNumber < 1) return 0.0;

    double weight_jump = 0;
    double weight_mark = 0;

    LastPtStatus lastPtStatus;
    double jumpCnt = 0, markCnt = 0;
    for (const auto &partIndex : validList)
    {
        calcBorderPathsWeight(*_buildPartMap[partIndex]->_solidPath.lpPath_Cur, weight_jump, weight_mark,
                              jumpCnt, markCnt, lastPtStatus);
    }

    auto refSpeed = (double)_writeBuff->_bppParaPtr->sHatchingPara.nMarkSpeed[0];
    double factor = 0.0;
    double factor_jump = _writeBuff->_bppParaPtr->sHatchingPara.fLineSpacing[0] * 0.1 * refSpeed;
    for (int i = 0; i < _writeBuff->_bppParaPtr->sBorderPara.nNumber; ++ i)
    {
        factor += _writeBuff->_bppParaPtr->sHatchingPara.fLineSpacing[0] * 1000 *
                  refSpeed / _writeBuff->_bppParaPtr->sBorderPara.nMarkSpeed[i];
    }
    return weight_jump * factor_jump + weight_mark * factor/* + jumpCnt * 0.00005 + markCnt * 0.00002*/;
}

double DividerProcessor::calcSupportWeight(const Paths &paths)
{
    auto _writeBuff = _slaPriv->_writerBufferParas.data();
    double weight_jump = 0;
    double weight_mark = 0;
    double jumpCnt = 0, markCnt = 0;
    LastPtStatus lastPtStatus;
    calcSupportPathsWeight(paths, weight_jump, weight_mark, jumpCnt, markCnt, lastPtStatus);

    auto refSpeed = (double)_writeBuff->_bppParaPtr->sHatchingPara.nMarkSpeed[0];
    double factor = _writeBuff->_bppParaPtr->sHatchingPara.fLineSpacing[0] * 1000 *
                    refSpeed / _writeBuff->_bppParaPtr->sSupportPara.nMarkSpeed;
    double factor_jump = _writeBuff->_bppParaPtr->sHatchingPara.fLineSpacing[0] * 0.16 * refSpeed;
    return weight_jump * factor_jump + weight_mark * factor/* + jumpCnt * 0.00005 + markCnt * 0.00002*/;
}

double DividerProcessor::calcUpfaceWeight(const Paths &paths, const int &up_index)
{
    BoundingBox bBox;
    auto area = Areas(paths, bBox);
    auto maxArea = (bBox.maxY - bBox.minY) * (bBox.maxX - bBox.minX);

    auto _writeBuff = _slaPriv->_writerBufferParas.data();
    double times = (_writeBuff->_bppParaPtr->sSurfacePara_Up.nHatchingType[up_index] == 3) ? 2.0 : 1.0;
    double factor1 = _writeBuff->_bppParaPtr->sHatchingPara.fLineSpacing[0] /
                     _writeBuff->_bppParaPtr->sSurfacePara_Up.fLineSpacing[up_index];
    double factor2 = (double)_writeBuff->_bppParaPtr->sHatchingPara.nMarkSpeed[0] /
                    _writeBuff->_bppParaPtr->sSurfacePara_Up.nMarkSpeed[up_index];
    return (area * (1 + (maxArea - area) / maxArea) * factor1 * factor2) * 1.25 * times;
}

double DividerProcessor::calcDownfaceWeight(const Paths &paths, const int &down_index)
{
    BoundingBox bBox;
    auto area = Areas(paths, bBox);
    auto maxArea = (bBox.maxY - bBox.minY) * (bBox.maxX - bBox.minX);


    auto _writeBuff = _slaPriv->_writerBufferParas.data();
    double times = (_writeBuff->_bppParaPtr->sSurfacePara_Up.nHatchingType[down_index] == 3) ? 2.0 : 1.0;
    double factor1 = _writeBuff->_bppParaPtr->sHatchingPara.fLineSpacing[0] /
                     _writeBuff->_bppParaPtr->sSurfacePara_Dw.fLineSpacing[down_index];
    double factor2 = (double)_writeBuff->_bppParaPtr->sHatchingPara.nMarkSpeed[0] /
                     _writeBuff->_bppParaPtr->sSurfacePara_Dw.nMarkSpeed[down_index];
    return (area * (1 + (maxArea - area) / maxArea) * factor1 * factor2) * 1.25 * times;
}

void DividerProcessor::runTaskInConcurrent(const QSet<int> &totalKeys,
                                           const QList<int> &supportKeys,
                                           const QList<int> &solidKeys,
                                           const QMap<int, Paths> &validSupportPartMap)
{
    auto q = _slaPriv->q_ptr;
    if (false == q->isRunning()) return;
    // _slaPriv->futureWatcher.setFuture(QtConcurrent::map(totalKeys,
    //                                                     std::bind(&DividerProcessor::taskRunner, this, std::placeholders::_1, supportKeys, solidKeys, validSupportPartMap)));
    // _slaPriv->futureWatcher.waitForFinished();

    const auto keys = totalKeys.toList();
#pragma omp parallel for
    for (int i = 0; i < keys.size(); ++ i)
    {
        taskRunner(keys.at(i), supportKeys, solidKeys, validSupportPartMap);
    }
}

void DividerProcessor::taskRunner(const int &partIndex, const QList<int> &supportKeys,
                                  const QList<int> &solidKeys, const QMap<int, Paths> &validSupportPartMap)
{
    auto uspWriter = _buildPartMap[partIndex]->_fileWriterPtr;
    SOLIDPATH &solidPath = _buildPartMap[partIndex]->_solidPath;
    auto varioLayerAreaFactor = _slaPriv->_selfAdaptiveModule->getMarkSpeedRatio(_buildPartMap[partIndex]->_fLayerThickness);

#ifdef Calc_Support
    if (supportKeys.contains(partIndex))
    {
        uspWriter->algo()->writeSupportToSList(validSupportPartMap[partIndex], varioLayerAreaFactor.fSupportFactor);
    }
#endif

    if (solidKeys.contains(partIndex))
    {
        uspWriter->algo()->createDistributionArea(_divideDataMap[partIndex]->_paths_Inner, DistAreaType::Hatching);
        uspWriter->algo()->createDistributionArea(_divideDataMap[partIndex]->_paths_up, DistAreaType::Upface);
        uspWriter->algo()->createDistributionArea(_divideDataMap[partIndex]->_paths_down, DistAreaType::Downface);

#ifdef Calc_Solid
        uspWriter->algo()->writeDivideData(_divideDataMap[partIndex], varioLayerAreaFactor.fHatchiongFactor,
                                           varioLayerAreaFactor.fLargeLineSpaceFactor,
                                           varioLayerAreaFactor.fSmallLineSpaceFactor);
#endif

#ifdef Calc_Border
        uspWriter->algo()->writeBorderData(solidPath.paths_exceptHolesAndGaps, varioLayerAreaFactor.fBorderFactor);
        uspWriter->algo()->writeHoleAndGapData(solidPath.paths_smallHoles,
                                               solidPath.paths_smallGaps, varioLayerAreaFactor.fBorderFactor);
        uspWriter->algo()->writeExternedBorder(_divideDataMap[partIndex]->_extendPaths, varioLayerAreaFactor.fBorderFactor);
#endif
        uspWriter->algo()->resetDistributionInfo();
        _divideDataMap[partIndex]->resetValues();
    }

    if(solidPath.lpPath_Cur && solidPath.lpPath_Cur->size())
    {
        uspWriter->algo()->addHatchingAngle();
    }
    uspWriter->waitBuffWriter();
}
