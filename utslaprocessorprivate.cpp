#include "utslaprocessorprivate.h"
#include "parameterscontrol.h"
#include "sljobfilewriter.h"
#include "utbpparareader.h"
#include "uspfilewriter.h"
#include "qjsonparsing.h"
#include "bpccommon.h"
#include "ziplib.h"

#include "SelfAdaptiveModule/selfadaptivemodule.h"
#include "LatticeModule/latticeinterface.h"

#include <QElapsedTimer>
#include <QtConcurrent>
#include <QThread>
#include <atomic>

#include "sljobfilewriter.h"
#include "DynamicDivider/dividerprocessor.h"

#define MaxReadSz 10240000L

///
/// @brief UTSLAProcessor私有实现类的构造函数
/// @param q UTSLAProcessor对象指针
/// @details 实现步骤:
///   1. 初始化处理器指针
///   2. 创建自适应模块
///   3. 创建写入缓冲区参数
///   4. 设置线程数量
///
UTSLAProcessorPrivate::UTSLAProcessorPrivate(UTSLAProcessor *q)
    : q_ptr(q)
{
    // 创建自适应模块实例
    _selfAdaptiveModule = QSharedPointer<SelfAdaptiveModule>(new SelfAdaptiveModule);

    // 创建写入缓冲区参数实例
    _writerBufferParas = QSharedPointer<WriterBufferParas>(new WriterBufferParas);

    // 设置线程数为CPU核心数-1,确保至少有1个线程
    _threadCount = QThread::idealThreadCount() - 1;
    if (_threadCount < 1) _threadCount = 1;
}

///
/// @brief 执行处理流程
/// @param nSLayer [in] 起始层高度(未使用)
/// @param nELayer [in] 结束层高度(未使用) 
/// @param jsonObj [in] JSON配置对象
/// @details 实现步骤:
///   1. 加载扩展包参数
///   2. 创建层厚列表
///   3. 获取扫描器配置
///   4. 根据扫描范围模式选择处理方式:
///      - 多扫描器分区处理
///      - 普通构建处理
///   5. 生成处理结果
///   6. 根据导出模式创建结果文件
///
void UTSLAProcessorPrivate::processing(const int &nSLayer, const int &nELayer, const QJsonObject &jsonObj)
{
    Q_UNUSED(nSLayer) Q_UNUSED(nELayer)
    QElapsedTimer elapsed;
    elapsed.start();

    // 加载扩展包参数
    if(jsonObj.contains("packageExtend"))
    {
        auto bpxObj = jsonObj["packageExtend"].toObject();
        if(bpxObj.contains("url"))
        {
            if(false == UTBPParaReader::loadParameters(bpxObj["url"].toString(), _selfAdaptiveModule->getSelfAdaptive()))
            {
                q_ptr->setProcessStatus(getBPCMsg("load bpx file error!", 209), false);
                return;
            }
        }
    }
    // 创建层厚列表
    _selfAdaptiveModule->createThicknessList();

    // 获取扫描器配置
    auto scannerCnt = _writerBufferParas->getExtendedValue<int>("Splicing/nNumber_SplicingScanner", 1);
    auto scanRangeMode = _writerBufferParas->getExtendedValue<int>("Splicing/nScanRangeMode", 0);

    // 选择处理方式
    _progressPos = -1;
    if (1 == scanRangeMode && scannerCnt > 1)
    {
        DividerProcessor divederProcesser(this);
        divederProcesser.startProcessing();
    }
    else buildProcessing();


    Q_Q(UTSLAProcessor);
    if(false == q->isRunning()) return;

    // 生成处理结果
    QJsonObject jsonResult;
    jsonResult.insert("code", 11);
    jsonResult.insert("status", "finished");
    jsonResult.insert("progress", "100");
    jsonResult.insert("message", "finished!");

    // 创建结果文件
    if(EXPORT_SLM_BUILDFILE == nExportFileMode) createJobFileResult(jsonResult);
    else
    {
        if(q->needCompression()) createJsonResultWithCompress(jsonResult);
        else createJsonResult(jsonResult);
    }
    qDebug() << "processing elapsed" << elapsed.elapsed();
}

void UTSLAProcessorPrivate::stopBuildProcessing()
{
    futureWatcher.cancel();
}

///
/// @brief 加载参数配置
/// @param strBppPath [in] BPP文件路径
/// @param strIniPath [in] INI文件路径
/// @return 加载是否成功
/// @details 实现步骤:
///   1. 获取BPP文件名
///   2. 加载BPP参数
///   3. 加载客户参数
///   4. 检查平台尺寸
///   5. 重置和添加扩展参数
///   6. 加载导出参数
///   7. 初始化点阵结构
///
bool UTSLAProcessorPrivate::loadParameters(const QString &strBppPath, const QString &strIniPath)
{
    Q_Q(UTSLAProcessor);

    // 获取BPP文件名
    strBPPName = QFileInfo(strBppPath).completeBaseName();

    // 加载BPP参数
    if(false == UTBPParaReader::loadParameters(strBppPath, *(_writerBufferParas->_bppParaPtr.data()),
                                             &_writerBufferParas->_extendObj))
    {
        q->setProcessStatus(getBPCMsg("load bpp error!", 210), false);
        return false;
    }

    // 加载客户参数
    if(false == ParametersControl::loadPara_Customer(*(_writerBufferParas->_bpcParaPtr.data()), strIniPath))
    {
        q->setProcessStatus(getBPCMsg("load ini error!", 211), false);
        return false;
    }

    // 检查平台尺寸
    if(0 == _writerBufferParas->_bpcParaPtr->nPlatWidthX || 0 == _writerBufferParas->_bpcParaPtr->nPlatWidthY)
    {
        q->setProcessStatus(getBPCMsg(QString("Platform info error!(%1 %2)").
                                   arg(_writerBufferParas->_bpcParaPtr->nPlatWidthX).
                                   arg(_writerBufferParas->_bpcParaPtr->nPlatWidthY), 212), false);
        return false;
    }

    // 重置和添加扩展参数
    _writerBufferParas->_extendedParaPtr->resetValue(_writerBufferParas->_bpcParaPtr->mExtendParasObj);
    _writerBufferParas->_extendedParaPtr->addVariantMap(_writerBufferParas->_bpcParaPtr->extendedParas);

    // 加载导出参数并初始化点阵
    loadExportParameters();
    initializeLattice();

    qDebug() << "!!!!!!!!!!!!!! loadParameters success!!!!!!!!!!!!!!!!";
    return true;
}


///
/// @brief 执行指定层范围的构建过程
/// @param nSLayer [in] 起始层高度
/// @param nELayer [in] 结束层高度
/// @details 实现步骤:
///   1. 从起始层开始遍历到结束层
///   2. 对每个有效层读取并处理数据
///   3. 检查构建过程是否需要继续
///
void UTSLAProcessorPrivate::building(const int &nSLayer, const int &nELayer)
{
    Q_Q(UTSLAProcessor);

    // 从起始层遍历到结束层
    int nCurLayer = nSLayer;
    while(q->isRunning() && nCurLayer < nELayer)
    {
        // 处理有效层数据
        if(q->isEffectiveLayer(nCurLayer))
        {
            readDatas(nCurLayer);
        }
        ++ nCurLayer;
    }
}


///
/// @brief 处理单个零件的构建过程
/// @param index [in] 零件索引
/// @details 实现步骤:
///   1. 创建文件写入器
///   2. 获取层范围和进度步长
///   3. 写入文件头信息
///   4. 定义层处理函数
///   5. 根据线程数选择处理模式:
///      - 单线程直接处理
///      - 多线程分块并行处理
///   6. 写入文件结束信息
///   7. 创建USP文件
///
void UTSLAProcessorPrivate::partProcessing(const int &index)
{
    Q_Q(UTSLAProcessor);
    if (false == q->isRunning()) return;

    // 创建文件写入器
    USPFileWriterPtr uspWriter = nullptr;
    if (false == createFileWriter(index, uspWriter)) return;

    // 获取层范围
    int beginLayer = q->getMinLayer(index);
    const int nEndLayer = q->getMaxLayer(index) + 1;

#ifdef USE_PROCESSOR_EXTEND
    beginLayer = 600;
    nEndLayer = 601;
#endif
    // 计算进度步长
    int nTotalLayer = nEndLayer - beginLayer - 1;
    double fStep = 1.0 / nTotalLayer * fSinglePartRatio;

    // 写入文件头
    uspWriter->writeFileBegin_SomePropertys();

    // 定义层处理函数
    auto funcRunner = [this, &index, &fStep](const int &beginLayer, const int &nEndLayer, const USPFileWriterPtr &uspWriter) {
        Q_Q(UTSLAProcessor);
        int nCurLayer = beginLayer;
        int nLastLayer = beginLayer - 1;

#ifdef MaxScanLayers
        int tempLayerIndex = 0;
#endif
        SOLIDPATH solidPath;

        while(q->isRunning() && nCurLayer < nEndLayer)
        {
            if(q->isEffectiveLayer(nCurLayer, index))
            {
#ifdef USE_PROCESSOR_EXTEND
                glProcessor->clearCanvas(nCurLayer);
#endif
                calcLayerDatas(nCurLayer, index, nCurLayer - nLastLayer, uspWriter, solidPath);
                updateProgress(double(nCurLayer - nLastLayer) * fStep);
                nLastLayer = nCurLayer;
#ifdef MaxScanLayers
                if(++ tempLayerIndex >= MaxScanLayers) break;
#endif
            }
            ++ nCurLayer;
        }
    };

    // 获取线程数
    int runnerNum = _writerBufferParas->getExtendedValue<int>("Lattice/runnerNum", 1);
    if (runnerNum < 1) runnerNum = 1;
    if (runnerNum > 32) runnerNum = 32;

    if (1 == runnerNum)
    {
        // 单线程处理
        funcRunner(beginLayer, nEndLayer, uspWriter);
    }
    else
    {
        // 多线程并行处理
        // 定义子文件信息结构
        struct SubFileInfo {
            int _beginLayer = 0;
            int _endLayer = 0;
            QString _subFileName = "";
        };
        typedef QSharedPointer<SubFileInfo> SubFileInfoPtr;
        QVector<SubFileInfoPtr> subFileVec;
        std::atomic<double> totalVolume;

        // 创建临时文件夹
        int curLayerHei = beginLayer;
        const int layerStep = 8000;
        auto saveFolder = q->getSaveFolder() + QUuid::createUuid().toString().remove(QRegExp("[{}-]")) + "/";
        QDir dir(saveFolder);
        if (false == dir.exists()) dir.mkpath(saveFolder);

        // 获取子文件信息
        QMutex subFileLocker;
        auto getSubFileInfo = [&]() -> const SubFileInfoPtr {
            QMutexLocker locker(&subFileLocker);
            if (curLayerHei >= nEndLayer) return nullptr;

            auto subFilePtr = SubFileInfoPtr(new SubFileInfo);
            auto endLayer = curLayerHei + layerStep;
            if (endLayer > nEndLayer) endLayer = nEndLayer;
            subFilePtr->_beginLayer = curLayerHei;
            subFilePtr->_endLayer = endLayer;
            subFilePtr->_subFileName = saveFolder + QUuid::createUuid().toString().remove(QRegExp("[{}-]")) + ".subtemp";
            curLayerHei = endLayer + 1;
            subFileVec << subFilePtr;
            return subFilePtr;
        };

        
        auto threadFunc = [&]() {
            while (q->isRunning()) {
                auto subFilePtr = getSubFileInfo();
                if (nullptr == subFilePtr || (subFilePtr->_beginLayer >= subFilePtr->_endLayer)) break;
                auto writer = uspWriter->cloneWriter();
                createFileWriter(index, subFilePtr->_subFileName, writer);
                funcRunner(subFilePtr->_beginLayer, subFilePtr->_endLayer, writer);
                if (auto tempWriter = dynamic_cast<USPFileWriter *>(writer.data())) {
                    totalVolume = totalVolume + tempWriter->m_fTotalVolume;
                }
                writer->getBufPara()->gFile.close();
            }
        };

        // 并行执行线程
#pragma omp parallel for num_threads(runnerNum)
        for (int i = 0; i < runnerNum; ++i) threadFunc();

        // 合并子文件
        for (const auto &subFileInfo : qAsConst(subFileVec))
        {
            QFile file(subFileInfo->_subFileName);
            if (file.open(QIODevice::ReadOnly))
            {
                auto leftSz = file.size();
                while (false == file.atEnd())
                {
                    uspWriter->getBufPara()->gFile.write(file.read(leftSz > MaxReadSz ? MaxReadSz : leftSz));
                    leftSz -= MaxReadSz;
                }
                file.close();
                file.remove();
            }
        }
        // 清理临时文件夹并更新体积
        dir.rmpath(saveFolder);
        uspWriter->addVolume(totalVolume);
    }
    // 写入文件结束信息
    uspWriter->writeFileEnd();

    // 创建USP文件
    if(q->isRunning())
    {
        QJsonObject jsonPartInfo;
        uspWriter->createUSPFile(q->getBuildPartName(index), strBPPName, q->getLayerThickness(index),
                                 q->getBoundingBox(index), q->getMinLayer(index), q->getMaxLayer(index), jsonPartInfo);
        objZipDescFile << jsonPartInfo;
    }
}

///
/// @brief 执行构建处理
/// @details 实现步骤:
///   1. 更新层拼接信息
///   2. 获取所有零件的层范围
///   3. 计算单个零件的进度比例
///   4. 并行处理所有零件
///   5. 更新最终进度
///
void UTSLAProcessorPrivate::buildProcessing()
{
    Q_Q(UTSLAProcessor);

    // 更新层拼接信息
    updateLayerSplicingInfo();

    // 获取所有零件的层范围
    int partCnt = q->getBuildPartCount();
    int curLayer = _CRT_INT_MAX;
    int endLayer = -1;

    for(int i = 0; i < partCnt; ++ i)
    {
        curLayer = qMin(curLayer, q->getMinLayer(i));
        endLayer = qMax(endLayer, q->getMinLayer(i));
    }

    // 计算单个零件进度比例
    fSinglePartRatio = 100.0 / q->getBuildPartCount();

    // 并行处理所有零件
    QVector<int> listInt;
    int nCnt = q->getBuildPartCount();
    for(int iCnt = 0; iCnt < nCnt; ++ iCnt) 
        listInt << iCnt;

    futureWatcher.setFuture(QtConcurrent::map(listInt, std::bind(&UTSLAProcessorPrivate::partProcessing,
                                                                this, std::placeholders::_1)));
    futureWatcher.waitForFinished();

    // 更新最终进度
    if(fabs(100.0 - fProcessing) > 1E-6) 
        updateProgress(100.0 - fProcessing);
}


///
/// @brief 读取并解析指定层的所有数据
/// @param nHei [in] 层高度
/// @details 实现步骤:
///   1. 读取所有类型的层数据
///   2. 计算SLM缩放
///   3. 解析数据
///
void UTSLAProcessorPrivate::readDatas(const int &nHei)
{
    Q_Q(UTSLAProcessor);

    // 读取所有类型的层数据
    QList<AREAINFOPTR> listAreaPtr;
    q->readLayerDatas(nHei, listAreaPtr, FTYPE_ALL);

    // 计算SLM缩放
    calcSLMScale(listAreaPtr);

    // 解析数据
    parsingDatas(nHei, listAreaPtr);
}


///
/// @brief 计算层数据并写入文件
/// @param nHei [in] 当前层高度
/// @param index [in] 零件索引
/// @param layerThickness [in] 层厚类型
/// @param uspWriter [in] 文件写入器
/// @param solidPath [in,out] 实体路径数据
/// @details 实现步骤:
///   1. 计算实际层厚
///   2. 读取实体和支撑数据
///   3. 计算边界矩形
///   4. 生成点阵结构
///   5. 计算面积并写入层信息
///   6. 处理孔洞和间隙
///   7. 写入各类扫描数据
///
void UTSLAProcessorPrivate::calcLayerDatas(const int &nHei, const int &index, const int &layerThickness,
                                          const USPFileWriterPtr &uspWriter, SOLIDPATH &solidPath)
{
    Q_Q(UTSLAProcessor);

    // 计算实际层厚
    double fLayerThickness = 0.1;
    if(1 == layerThickness)
    {
        fLayerThickness = double(q->getLayerThickness(index));
    }
    else
    {
        fLayerThickness = layerThickness * LAYERHEIFACTOR;
    }

    // 启动写入并设置当前层
    uspWriter->startBuffWriter();
    uspWriter->algo()->setCurLayerHei(nHei);

    // 读取实体和支撑数据
    QList<AREAINFOPTR> areaSupport;
    readLayerDatas(nHei, index, uspWriter, solidPath);
    q->readLayerDatas(nHei, areaSupport, index, FTYPE_SUPPORT);
    calcSLMScale(areaSupport);

    // 读取实体支撑数据
    QList<AREAINFOPTR> listSolidSupportArea;
    Paths solidSupportPaths;
    q->readLayerDatas(nHei, listSolidSupportArea, index, FTYPE_SOLIDSUPPORT);
    if (nullptr == _latticeInfPtr) calcSLMScale(listSolidSupportArea);
    uspWriter->algo()->getAllPaths(listSolidSupportArea, solidSupportPaths);

    // 计算总边界矩形
    BOUNDINGRECT totalRc;
    uspWriter->algo()->calcLimitXY(areaSupport, totalRc);
    uspWriter->algo()->calcLimitXY(*solidPath.lpPath_Cur, totalRc);
    if (nullptr == _latticeInfPtr) uspWriter->algo()->calcLimitXY(solidSupportPaths, totalRc);

    // 生成点阵结构
    Paths lattices;
    LatticeLayerInfo latticeLayerInfo;
    if (solidPath.lpPath_Cur->size())
    {
        createLattice(*solidPath.lpPath_Cur, solidSupportPaths, nHei, lattices, latticeLayerInfo);
    }

    // 计算面积并写入层信息
    double fArea = 0.0;
    if(EXPORT_SLM_BUILDFILE == nExportFileMode)
    {
        if(solidPath.lpPath_Cur->size())
        {
            uspWriter->algo()->addHatchingAngle();
            fArea = ClipperLib::Areas(*solidPath.lpPath_Cur) * 1E-6;
        }
    }
    uspWriter->writeLayerInfo(nHei, totalRc, fArea);

    // 处理孔洞和间隙
    uspWriter->algo()->keepHolesAndGaps(solidPath.lpPath_Cur, solidPath.paths_smallHoles, solidPath.paths_smallGaps,
                                       solidPath.paths_exceptHolesAndGaps);

    // 计算扩展路径和拼接向量
    Paths extendPaths;
    uspWriter->algo()->calcSplicingVec();
    if(solidPath.lpPath_Cur->size())
    {
        double fArea = Areas(*solidPath.lpPath_Cur) * AREAFACTOR;
        uspWriter->addVolume(fArea * fLayerThickness);
        uspWriter->writeAreaInfo(int(fArea));
    }

    // 获取标记速度比率并写入各类数据
    auto varioLayerAreaFactor = _selfAdaptiveModule->getMarkSpeedRatio(fLayerThickness);
    
    // 写入实体支撑数据
    if (nullptr == _latticeInfPtr && solidSupportPaths.size())
    {
        uspWriter->algo()->writeSolidSupportData(solidSupportPaths, varioLayerAreaFactor.fBorderFactor,
                                                varioLayerAreaFactor.fHatchiongFactor,
                                                varioLayerAreaFactor.fLargeLineSpaceFactor,
                                                varioLayerAreaFactor.fSmallLineSpaceFactor);
        uspWriter->algo()->addSolidSupportHatchingAngle();
    }

    // 写入填充数据
    uspWriter->algo()->writeHatchingData(solidPath, extendPaths, varioLayerAreaFactor.fHatchiongFactor,
                                        varioLayerAreaFactor.fLargeLineSpaceFactor,
                                        varioLayerAreaFactor.fSmallLineSpaceFactor);

    // 写入点阵数据
    if (_latticeInfPtr)
    {
        uspWriter->algo()->writeHatchingData(lattices, varioLayerAreaFactor.fHatchiongFactor,
                                            varioLayerAreaFactor.fLargeLineSpaceFactor,
                                            varioLayerAreaFactor.fSmallLineSpaceFactor);
        uspWriter->algo()->writeHatchingData(latticeLayerInfo, varioLayerAreaFactor.fHatchiongFactor,
                                            varioLayerAreaFactor.fLargeLineSpaceFactor,
                                            varioLayerAreaFactor.fSmallLineSpaceFactor);
    }

    // 写入支撑和边界数据
    uspWriter->algo()->writeSupportToSList(areaSupport, varioLayerAreaFactor.fSupportFactor);
    uspWriter->algo()->writeBorderData(solidPath.paths_exceptHolesAndGaps, varioLayerAreaFactor.fBorderFactor);
    uspWriter->algo()->writeHoleAndGapData(solidPath.paths_smallHoles,
                                          solidPath.paths_smallGaps, varioLayerAreaFactor.fBorderFactor);
    uspWriter->algo()->writeExternedBorder(extendPaths, varioLayerAreaFactor.fBorderFactor);

    // 非SLM模式添加填充角度
    if(EXPORT_SLM_BUILDFILE != nExportFileMode)
    {
        if(solidPath.lpPath_Cur && solidPath.lpPath_Cur->size())
        {
            uspWriter->algo()->addHatchingAngle();
        }
    }

    // 等待写入完成
    uspWriter->waitBuffWriter();
}


///
/// @brief 读取层数据并处理上下表面
/// @param nHei [in] 当前层高度
/// @param index [in] 零件索引
/// @param uspWriter [in] 文件写入器
/// @param solidPath [out] 实体路径数据
/// @details 实现步骤:
///   1. 获取下表面层索引列表
///   2. 获取上表面层索引列表
///   3. 清理并初始化路径数据
///   4. 处理当前层数据
///   5. 处理下表面层数据
///   6. 处理上表面层数据
///   7. 设置空路径指针
///
void UTSLAProcessorPrivate::readLayerDatas(const int &nHei, const int &index,
                                          const USPFileWriterPtr &uspWriter, SOLIDPATH &solidPath)
{
    Q_Q(UTSLAProcessor);
    auto *_writeBuff = uspWriter->getBufPara();

    // 获取下表面层索引
    QList<int> listIndex_Dw;
    DOWNSURFACEPARAMETERS *lpDownParas = &BppParas->sSurfacePara_Dw;
    if(lpDownParas->nNumber)
    {
        int nPreLayerHei = nHei;
        for(int iSur = 0; iSur < lpDownParas->nNumber; ++ iSur)
        {
            nPreLayerHei = getPreLayerHei(nPreLayerHei, index);
            if(nPreLayerHei < 0) break;
            listIndex_Dw << nPreLayerHei;
        }
    }

    // 获取上表面层索引
    QList<int> listIndex_Up;
    UPSURFACEPARAMETERS *lpUpParas = &BppParas->sSurfacePara_Up;
    if(lpUpParas->nNumber)
    {
        int nNextLayerHei = nHei;
        for(int iSur = 0; iSur < lpUpParas->nNumber; ++ iSur)
        {
            nNextLayerHei = getNextLayerHei(nNextLayerHei, index);
            if(nNextLayerHei < 0) break;
            listIndex_Up << nNextLayerHei;
        }
    }

    // 清理并初始化路径数据
    solidPath.clearAllPathPointer();
    for(int iLayer = 0; iLayer < 11; iLayer ++)
    {
        solidPath.nLayerUsed[iLayer] = 0;
        if(nHei == solidPath.sLayerDatas[iLayer].nLayerHei)
        {
            solidPath.nLayerUsed[iLayer] = 1;
            solidPath.curPaths = solidPath.sLayerDatas[iLayer].allPaths;
            solidPath.lpPath_Cur = &solidPath.curPaths;
        }
        else if(listIndex_Dw.contains(solidPath.sLayerDatas[iLayer].nLayerHei))
        {
            solidPath.nLayerUsed[iLayer] = 1;
            solidPath.lpPath_Dw[listIndex_Dw.indexOf(solidPath.sLayerDatas[iLayer].nLayerHei)] = &solidPath.sLayerDatas[iLayer].allPaths;
        }
        else if(listIndex_Up.contains(solidPath.sLayerDatas[iLayer].nLayerHei))
        {
            solidPath.nLayerUsed[iLayer] = 1;
            solidPath.lpPath_Up[listIndex_Up.indexOf(solidPath.sLayerDatas[iLayer].nLayerHei)] = &solidPath.sLayerDatas[iLayer].allPaths;
        }
    }

    // 处理当前层数据
    if(nullptr == solidPath.lpPath_Cur)
    {
        for(int iLayer = 0; iLayer < 11; iLayer ++)
        {
            if(0 == solidPath.nLayerUsed[iLayer])
            {
                QList<AREAINFOPTR> listAreaPtr;
                q->readLayerDatas(nHei, listAreaPtr, index, FTYPE_SOLID);
                calcSLMScale(listAreaPtr);

                solidPath.nLayerUsed[iLayer] = 1;
                solidPath.sLayerDatas[iLayer].nLayerHei = nHei;
                uspWriter->algo()->getAllPaths(listAreaPtr, solidPath.sLayerDatas[iLayer].allPaths);
                solidPath.curPaths = solidPath.sLayerDatas[iLayer].allPaths;
                solidPath.lpPath_Cur = &solidPath.curPaths;
                break;
            }
        }
    }

    // 处理下表面层数据
    for(int iSur = 0; iSur < lpDownParas->nNumber && iSur < listIndex_Dw.count(); iSur ++)
    {
        if(nullptr == solidPath.lpPath_Dw[iSur])
        {
            for(int iLayer = 0; iLayer < 11; iLayer ++)
            {
                if(0 == solidPath.nLayerUsed[iLayer])
                {
                    QList<AREAINFOPTR> listAreaPtr;
                    q->readLayerDatas(listIndex_Dw.at(iSur), listAreaPtr, index, FTYPE_SOLID);
                    calcSLMScale(listAreaPtr);

                    solidPath.nLayerUsed[iLayer] = 1;
                    solidPath.sLayerDatas[iLayer].nLayerHei = listIndex_Dw.at(iSur);
                    uspWriter->algo()->getAllPaths(listAreaPtr, solidPath.sLayerDatas[iLayer].allPaths);
                    solidPath.lpPath_Dw[iSur] = &solidPath.sLayerDatas[iLayer].allPaths;
                    break;
                }
            }
        }
    }

    // 处理上表面层数据
    for(int iSur = 0; iSur < lpUpParas->nNumber && iSur < listIndex_Up.count(); iSur ++)
    {
        if(nullptr == solidPath.lpPath_Up[iSur])
        {
            for(int iLayer = 0; iLayer < 11; iLayer ++)
            {
                if(0 == solidPath.nLayerUsed[iLayer])
                {
                    QList<AREAINFOPTR> listAreaPtr;
                    q->readLayerDatas(listIndex_Up.at(iSur), listAreaPtr, index, FTYPE_SOLID);
                    calcSLMScale(listAreaPtr);

                    solidPath.nLayerUsed[iLayer] = 1;
                    solidPath.sLayerDatas[iLayer].nLayerHei = listIndex_Up.at(iSur);
                    uspWriter->algo()->getAllPaths(listAreaPtr, solidPath.sLayerDatas[iLayer].allPaths);
                    solidPath.lpPath_Up[iSur] = &solidPath.sLayerDatas[iLayer].allPaths;
                    break;
                }
            }
        }
    }

    // 设置空路径指针
    for(int iSur = 0; iSur < 5; iSur ++)
    {
        if(nullptr == solidPath.lpPath_Dw[iSur])
        {
            solidPath.lpPath_Dw[iSur] = &solidPath.nullPaths;
        }
        if(nullptr == solidPath.lpPath_Up[iSur])
        {
            solidPath.lpPath_Up[iSur] = &solidPath.nullPaths;
        }
    }
}


///
/// @brief 获取上一个有效层高度
/// @param nStartLayer [in] 起始层高度
/// @param index [in] 零件索引
/// @return 上一个有效层高度，如果没有则返回-1
/// @details 实现步骤:
///   1. 从起始层开始向下遍历
///   2. 检查每一层是否为有效实体层
///   3. 返回找到的第一个有效层高度
///
int UTSLAProcessorPrivate::getPreLayerHei(const int &nStartLayer, const int &index)
{
    Q_Q(UTSLAProcessor);

    // 获取当前层和最小层
    int nCurLayer = nStartLayer;
    const int nMinLayer = q->getMinLayer(index) - 1;

    // 向下遍历查找有效层
    while(-- nCurLayer > nMinLayer)
    {
        // 找到有效实体层则返回
        if(q->isEffectiveLayer(nCurLayer, index, FTYPE_SOLID)) 
            return nCurLayer;
    }

    // 未找到有效层返回-1
    return -1;
}


///
/// @brief 获取下一个有效层高度
/// @param nStartLayer [in] 起始层高度
/// @param index [in] 零件索引
/// @return 下一个有效层高度，如果没有则返回-1
/// @details 实现步骤:
///   1. 从起始层开始向上遍历
///   2. 检查每一层是否为有效实体层
///   3. 返回找到的第一个有效层高度
///
int UTSLAProcessorPrivate::getNextLayerHei(const int &nStartLayer, const int &index)
{
    Q_Q(UTSLAProcessor);

    // 获取当前层和最大层
    int nCurLayer = nStartLayer;
    const int nMaxLayer = q->getMaxLayer(index) + 1;

    // 向上遍历查找有效层
    while(++ nCurLayer < nMaxLayer)
    {
        // 找到有效实体层则返回
        if(q->isEffectiveLayer(nCurLayer, index, FTYPE_SOLID)) 
            return nCurLayer;
    }

    // 未找到有效层返回-1
    return -1;
}


///
/// @brief 更新层拼接信息
/// @details 实现步骤:
///   1. 初始化拼接参数
///   2. 获取所有零件的层范围
///   3. 遍历所有有效层
///   4. 添加层拼接区域
///
void UTSLAProcessorPrivate::updateLayerSplicingInfo()
{
    Q_Q(UTSLAProcessor);

    // 初始化拼接参数
    _writerBufferParas->_splicingPtr->initializeParas(_writerBufferParas.data());

    // 获取零件数量和层范围
    int partCnt = q->getBuildPartCount();
    int curLayer = _CRT_INT_MAX;
    int endLayer = -1;

    // 计算所有零件的层范围
    for(int i = 0; i < partCnt; ++ i)
    {
        curLayer = qMin(curLayer, q->getMinLayer(i));
        endLayer = qMax(endLayer, q->getMaxLayer(i));
    }

    // 遍历所有层添加拼接区域
    endLayer += 1;
    while(curLayer < endLayer)
    {
        // 对有效层添加拼接区域
        if(q->isEffectiveLayer(curLayer))
        {
            _writerBufferParas->_splicingPtr->addLayerSplicingArea(curLayer);
        }
        ++ curLayer;
    }
}


void UTSLAProcessorPrivate::parsingDatas(const int &nHei, QList<AREAINFOPTR> &listAreaPtr)
{
#ifndef USE_PROCESSOR_EXTEND
    Q_UNUSED(nHei) Q_UNUSED(listAreaPtr)
#else
    Q_Q(UTSLAProcessor);
    q->clearCanvas(nHei);
    int nX1 = 0;
    int nY1 = 0;
    int nX2 = 0;
    int nY2 = 0;
    for(const auto &areaPtr : listAreaPtr)
    {
        if(areaPtr->nDir < 2)
        {
            uint nPtNum = areaPtr->ptPath.size();
            nX1 = int(areaPtr->ptPath[0].X);
            nY1 = int(areaPtr->ptPath[0].Y);
            for(uint iPt = 1; iPt < nPtNum; ++ iPt)
            {
                nX2 = int(areaPtr->ptPath[iPt].X);
                nY2 = int(areaPtr->ptPath[iPt].Y);
                q->drawLines(nX1, nY1, nX2, nY2, areaPtr->nDir);
                nX1 = nX2;
                nY1 = nY2;
            }
            if(nX1 != nX2 || nY1 != nY2)
            {
                nX2 = int(areaPtr->ptPath[0].X);
                nY2 = int(areaPtr->ptPath[0].Y);
                q->drawLines(nX1, nY1, nX2, nY2, areaPtr->nDir);
            }
        }
        else
        {
            uint nPtNum = areaPtr->ptPath.size();
            uint nLNum = nPtNum >> 1;
            for(uint iLine = 0; iLine < nLNum; ++ iLine)
            {
                nPtNum = (iLine << 1);
                nX1 = int(areaPtr->ptPath[nPtNum].X);
                nY1 = int(areaPtr->ptPath[nPtNum].Y);
                nX2 = int(areaPtr->ptPath[nPtNum + 1].X);
                nY2 = int(areaPtr->ptPath[nPtNum + 1].Y);
                q->drawLines(nX1, nY1, nX2, nY2, areaPtr->nDir);
            }
        }
    }
#endif
}

///
/// @brief 更新处理进度
/// @param temp 进度增量
/// @details 实现步骤:
///   1. 累加进度值
///   2. 计算进度位置
///   3. 检查是否需要更新
///   4. 构建进度信息
///   5. 发送进度更新
///
void UTSLAProcessorPrivate::updateProgress(const double &temp) 
{
    // 累加进度值
    fProcessing += temp;

    // 计算进度位置(放大20倍)
    auto tempProgress = qRound(fProcessing * 20);

    // 进度位置变化或超过2000时更新
    if (tempProgress != _progressPos || tempProgress > 2000)
    {
        // 更新进度位置
        _progressPos = tempProgress;

        // 获取处理器对象
        Q_Q(UTSLAProcessor);

        // 构建进度信息对象
        QJsonObject obj;
        obj.insert("code", 10);
        obj.insert("status", "running");
        obj.insert("message", "processing");
        obj.insert("progress", QString::number(fProcessing));

        // 发送进度更新
        q->updateProgress(obj);
    }
}


///
/// @brief 创建JSON格式的结果信息
/// @param jsonResult [out] 输出的JSON结果对象
/// @details 实现步骤:
///   1. 获取处理器对象和零件总数
///   2. 遍历所有零件文件
///   3. 为每个文件构建信息对象
///   4. 添加到结果数组
///   5. 更新处理进度
///
void UTSLAProcessorPrivate::createJsonResult(QJsonObject &jsonResult)
{
    // 获取处理器对象和零件数量
    Q_Q(UTSLAProcessor);
    QStringList listFile;
    int nTotalCnt = q->getBuildPartCount();

    // 构建结果数组
    QJsonArray resultArray;
    for(int iIndex = 0; iIndex < nTotalCnt; ++ iIndex)
    {
        // 构建单个文件信息
        QJsonObject jsonFileInfo;
        QString fileName = q->getSaveFolder() + q->getBuildPartName(iIndex) +
                         QString("_%1_%2.usp").arg(q->getSaveName()).arg(iIndex + 1);
        
        // 填充文件信息
        jsonFileInfo.insert("url", fileName);
        jsonFileInfo.insert("md5", QString(USPFileWriter::getFileHash(fileName)));
        jsonFileInfo.insert("fileType", "USP");
        jsonFileInfo.insert("fileId", q->getBuildPartName(iIndex));
        
        resultArray.append(jsonFileInfo);
    }

    // 更新结果和进度
    jsonResult.insert("targetFileList", resultArray);
    q->updateProgress(jsonResult);
}


///
/// @brief 创建压缩文件的JSON结果信息
/// @param jsonResult [out] 输出的JSON结果对象
/// @details 实现步骤:
///   1. 收集所有零件文件路径
///   2. 构建压缩文件路径
///   3. 创建ZIP压缩文件
///   4. 压缩成功后删除原文件
///   5. 生成压缩文件信息
///   6. 更新处理进度
///
void UTSLAProcessorPrivate::createJsonResultWithCompress(QJsonObject &jsonResult)
{
    // 获取处理器对象并收集文件路径
    Q_Q(UTSLAProcessor);
    QStringList listFile;
    int nTotalCnt = q->getBuildPartCount();
    for(int iIndex = 0; iIndex < nTotalCnt; ++ iIndex)
    {
        listFile << q->getSaveFolder() + q->getBuildPartName(iIndex) +
                    QString("_%1_%2.usp").arg(q->getSaveName()).arg(iIndex + 1);
    }

    // 构建压缩文件路径
    QString strSaveName = q->getSaveFolder() + q->getSaveName() + ".uzp";
    ZipLib zip;

    // 创建压缩描述信息
    QJsonObject objZipDesc;
    objZipDesc["PartInfo"] = objZipDescFile;

    // 执行压缩操作
    if(zip.CreateFilesZip(strSaveName, listFile, "UT20200725UT",
                         QString(QJsonDocument(objZipDesc).toJson()).toLocal8Bit()))
    {
        // 删除原始文件
        for(const auto &fileName : qAsConst(listFile))
        {
            QFile(fileName).remove();
        }

        // 构建压缩文件信息
        QJsonObject jsonTarget;
        jsonTarget.insert("url", q->getSaveName() + ".uzp");
        jsonTarget.insert("md5", QString(USPFileWriter::getFileHash(strSaveName)));
        jsonTarget.insert("fileType", "UZP");
        jsonTarget.insert("fileId", q->getSaveName());

        // 更新结果和进度
        jsonResult.insert("targetFile", jsonTarget);
        q->updateProgress(jsonResult);
    }
    else
    {
        // 压缩失败处理
        q->setProcessStatus(getBPCMsg("Compress Error!", 214), false);
    }
}

///
/// @brief 创建作业文件结果
/// @param jsonResult [out] 输出的JSON结果对象
/// @details 实现步骤:
///   1. 收集零件名称列表
///   2. 创建作业内容XML文件
///   3. 创建第一层压缩包(.job)
///   4. 删除原始XML文件
///   5. 创建最终压缩包(.zip)
///   6. 生成结果信息
///   7. 清理临时文件
///
void UTSLAProcessorPrivate::createJobFileResult(QJsonObject &jsonResult)
{
    // 获取处理器对象并收集零件名称
    Q_Q(UTSLAProcessor);
    QStringList listPartName;
    int nCnt = q->getBuildPartCount();
    for(int index = 0; index < nCnt; ++ index)
    {
        listPartName << q->getBuildPartName(index);
    }

    // 创建作业内容XML文件
    SLJobFileWriter::createJFileContentXml(q->getSaveFolder(), listPartName, q->getSaveName(), _writerBufferParas);

    // 准备第一层压缩文件列表
    QStringList listFile;
    listFile << (q->getSaveFolder() + "Content.xml") << (q->getSaveFolder() + "JobMetaData.job");
    {
        // 创建.job压缩包
        {
            ZipLib zip;
            zip.CreateFilesZip(q->getSaveFolder() + q->getSaveName() + ".job",
                             listFile, nullptr, "SLM Build File");
        }

        // 删除原始XML文件
        for(const QString &fileName : qAsConst(listFile))
        {
            QFile(fileName).remove();
        }

        // 准备最终压缩文件列表
        listFile.clear();
        listFile << q->getSaveFolder() + q->getSaveName() + ".job";
        for(const auto &name : qAsConst(listPartName))
        {
            listFile << (q->getSaveFolder() + name + ".bin");
        }

        // 创建最终.zip压缩包
        ZipLib zip;
        if(zip.CreateFilesZip(q->getSaveFolder() + q->getSaveName() + ".zip",
                             listFile, nullptr, "SLM Build File"))
        {
            // 构建结果信息
            QJsonObject jsonTarget;
            jsonTarget.insert("url", q->getSaveName() + ".zip");
            jsonTarget.insert("md5", QString(USPFileWriter::getFileHash(q->getSaveFolder() + q->getSaveName() + ".zip")));
            jsonTarget.insert("fileType", "zip");
            jsonTarget.insert("fileId", q->getSaveName());

            // 更新结果和进度
            jsonResult.insert("targetFile", jsonTarget);
            q->updateProgress(jsonResult);
        }
        else
        {
            // 压缩失败处理
            q->setProcessStatus(getBPCMsg("Compress Error!", 214), false);
        }

        // 清理临时文件
        for(const QString &fileName : qAsConst(listFile))
        {
            QFile(fileName).remove();
        }
    }
}

///
/// @brief 加载导出参数配置
/// @details 实现步骤:
///   1. 加载全局导出模式和缩放参数
///   2. 加载支撑填充参数
///   3. 加载支撑边界参数
///
void UTSLAProcessorPrivate::loadExportParameters()
{
    // 加载全局参数
    nExportFileMode = _writerBufferParas->_extendedParaPtr->getValue<int>("Global/nExportFileMode", EXPORT_NORMAL);
    fScaleX = _writerBufferParas->_extendedParaPtr->getValue<double>("SLMScale/fScaleX", 1.0);
    fScaleY = _writerBufferParas->_extendedParaPtr->getValue<double>("SLMScale/fScaleY", 1.0);

    auto *_writeBuff = _writerBufferParas.data();

    // 加载支撑填充参数
    {
        QJsonObject jsonObj;
        jsonObj["nPower"] = ExtendedParas<int>("SolidSupportHatching/nPower", 80);
        jsonObj["nMarkSpeed"] = ExtendedParas<int>("SolidSupportHatching/nMarkSpeed", 1000);
        jsonObj["nHatchingType"] = ExtendedParas<int>("SolidSupportHatching/nHatchingType", 0);
        jsonObj["fLineSpacing"] = ExtendedParas<double>("SolidSupportHatching/fLineSpacing", 0.01);
        jsonObj["fAngle"] = ExtendedParas<double>("SolidSupportHatching/fAngle", 60.3);
        _writeBuff->_extendObj.insert("SolidSupportHatching", jsonObj);
    }

    // 加载支撑边界参数
    {
        QJsonObject jsonObj;
        jsonObj["nUseBorder"] = ExtendedParas<int>("SolidSupportBorder/nUseBorder", 1);
        jsonObj["nPower"] = ExtendedParas<int>("SolidSupportBorder/nPower", 80);
        jsonObj["nMarkSpeed"] = ExtendedParas<int>("SolidSupportBorder/nMarkSpeed", 1000);
        _writeBuff->_extendObj.insert("SolidSupportBorder", jsonObj);
    }
}


///
/// @brief 计算SLM缩放
/// @param areaInfoList [in,out] 区域信息列表
/// @details 实现步骤:
///   1. 检查是否需要缩放处理
///   2. 检查缩放系数是否为1
///   3. 对所有区域进行坐标缩放
///
void UTSLAProcessorPrivate::calcSLMScale(QList<AREAINFOPTR> &areaInfoList)
{
    // 非SLM构建文件模式直接返回
    if(EXPORT_SLM_BUILDFILE != nExportFileMode) return;

    // 缩放系数接近1时无需处理
    if(fabs(1.0 - fScaleX) < 1E-6 && fabs(1.0 - fScaleY) < 1E-6) return;

    // 对所有区域进行XY坐标缩放
    for(auto &areaItem : areaInfoList)
    {
        for(auto coor = areaItem->ptPath.begin(); coor != areaItem->ptPath.end(); ++ coor)
        {
            // 分别对X、Y坐标进行缩放并四舍五入
            coor->X = qRound64(coor->X * fScaleX);
            coor->Y = qRound64(coor->Y * fScaleY);
        }
    }
}


///
/// @brief 创建文件写入器
/// @param index 零件索引
/// @param uspWriter [out] 文件写入器指针
/// @return 创建是否成功
/// @details 实现步骤:
///   1. 根据导出模式创建写入器
///   2. 设置扫描器索引
///   3. 初始化文件路径
///   4. 设置缓冲区参数
///   5. 初始化写入器
///   6. 检查平台参数
///   7. 设置导出模式
///
bool UTSLAProcessorPrivate::createFileWriter(const int &index, USPFileWriterPtr &uspWriter)
{
    Q_Q(UTSLAProcessor);

    // 根据导出模式创建写入器
    if(EXPORT_SLM_BUILDFILE == nExportFileMode)
    {
        // 创建SLM作业文件写入器
        uspWriter = USPFileWriterPtr(new SLJobFileWriter());
        
        // 设置扫描器索引
        auto boundingBox = q->getBoundingBox(index);
        if (boundingBox) 
            uspWriter->setScannerIndex(_writerBufferParas->_bpcParaPtr->getCurScannerIndex(
                (boundingBox->minX + boundingBox->maxX) * 0.5 * UNITFACTOR,
                (boundingBox->minY + boundingBox->maxY) * 0.5 * UNITFACTOR));
    }
    else 
        uspWriter = USPFileWriterPtr(new USPFileWriter());

    // 初始化文件路径
    QString strBuildName = q->getBuildPartName(index);
    uspWriter->getBufPara()->clearFileDataList();

    if(EXPORT_SLM_BUILDFILE == nExportFileMode)
    {
        uspWriter->getBufPara()->gFile.setFileName(q->getSaveFolder() + strBuildName + ".bin");
    }
    else
    {
        uspWriter->getBufPara()->gFile.setFileName(q->getSaveFolder() + strBuildName +
                                               QString("_%1_%2.tmp").arg(q->getSaveName()).arg(index + 1));
    }

    // 设置缓冲区参数
    uspWriter->getBufPara()->_writerBufferParas = _writerBufferParas;
    auto *_writeBuff = uspWriter->getBufPara();
    Q_UNUSED(_writeBuff)

    // 初始化写入器
    if(false == uspWriter->initFileWrite())
    {
        q->setProcessStatus(getBPCMsg("create file error!", 213), false);
        return false;
    }

    // 检查平台参数
    if(0 == BpcParas->nPlatWidthX || 0 == BpcParas->nPlatWidthY)
    {
        q->setProcessStatus(getBPCMsg(QString("Platform info error!(%1 %2)").
                                  arg(BpcParas->nPlatWidthX).
                                  arg(BpcParas->nPlatWidthY), 212), false);
        return false;
    }

    // 设置导出模式
    uspWriter->algo()->setExportFileMode(nExportFileMode);
    uspWriter->algo()->createEnhanceBorderParas();

    return true;
}


///
/// @brief 创建指定文件名的文件写入器
/// @param index 零件索引
/// @param strBuildName 构建文件名
/// @param uspWriter [out] 文件写入器指针
/// @return 创建是否成功
/// @details 实现步骤:
///   1. 根据导出模式创建写入器类型
///   2. 设置扫描器索引和文件名
///   3. 初始化写入器参数
///   4. 检查平台尺寸参数
///   5. 配置导出模式参数
///
bool UTSLAProcessorPrivate::createFileWriter(const int &index, const QString &strBuildName, USPFileWriterPtr &uspWriter)
{
    Q_Q(UTSLAProcessor);

    // 根据导出模式创建对应类型的写入器
    if(EXPORT_SLM_BUILDFILE == nExportFileMode)
    {
        uspWriter = USPFileWriterPtr(new SLJobFileWriter());
        auto boundingBox = q->getBoundingBox(index);
        uspWriter->setScannerIndex(_writerBufferParas->_bpcParaPtr->getCurScannerIndex(
            (boundingBox->minX + boundingBox->maxX) * 0.5 * UNITFACTOR,
            (boundingBox->minY + boundingBox->maxY) * 0.5 * UNITFACTOR));
    }
    else 
        uspWriter = USPFileWriterPtr(new USPFileWriter());

    // 清理文件列表并设置文件名
    uspWriter->getBufPara()->clearFileDataList();
    uspWriter->getBufPara()->gFile.setFileName(strBuildName);
    uspWriter->getBufPara()->_writerBufferParas = _writerBufferParas;

    auto *_writeBuff = uspWriter->getBufPara();
    Q_UNUSED(_writeBuff)

    // 初始化写入器
    if(false == uspWriter->initFileWrite())
    {
        q->setProcessStatus(getBPCMsg("create file error!", 213), false);
        return false;
    }

    // 检查平台尺寸参数
    if(0 == BpcParas->nPlatWidthX || 0 == BpcParas->nPlatWidthY)
    {
        q->setProcessStatus(getBPCMsg(QString("Platform info error!(%1 %2)").
                                  arg(BpcParas->nPlatWidthX).
                                  arg(BpcParas->nPlatWidthY), 212), false);
        return false;
    }

    // 设置导出模式和增强边界参数
    uspWriter->algo()->setExportFileMode(nExportFileMode);
    uspWriter->algo()->createEnhanceBorderParas();

    return true;
}


///
/// @brief 初始化点阵结构
/// @details 实现步骤:
///   1. 检查是否启用点阵功能
///   2. 创建点阵接口实例
///   3. 初始化点阵参数
///
void UTSLAProcessorPrivate::initializeLattice()
{
    // 检查是否启用点阵功能
    if (_writerBufferParas->_extendedParaPtr->getValue<int>("Lattice/useLattice", 0))
    {
        // 创建点阵接口实例
        _latticeInfPtr = QSharedPointer<LatticeInterface>(new LatticeInterface);
        
        // 使用扩展参数初始化点阵
        _latticeInfPtr->initLatticeParas(_writerBufferParas->_extendedParaPtr.data());
    }
}


///
/// @brief 创建点阵结构
/// @param paths [in] 输入路径
/// @param refPaths [in] 参考路径
/// @param layerHei [in] 层高
/// @param lattices [out] 生成的点阵路径
/// @param latticeLayerInfo [out] 点阵层信息
/// @details 实现步骤:
///   1. 检查点阵接口是否存在
///   2. 调用接口计算当前层的点阵结构
///
void UTSLAProcessorPrivate::createLattice(Paths &paths, const Paths &refPaths, const int &layerHei,
                                         Paths &lattices, LatticeLayerInfo &latticeLayerInfo)
{
    // 存在点阵接口时计算点阵结构
    if (_latticeInfPtr)
    {
        _latticeInfPtr->calcLayerLattic(paths, refPaths, layerHei, lattices, latticeLayerInfo);
    }
}
