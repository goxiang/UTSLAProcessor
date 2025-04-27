#include "sljobfilewriter.h"

#include "algorithmapplication.h"
#include "defnum2qstring.h"
#include "sljfilewriter.h"
#include "publicheader.h"
#include "writeuff.h"

#include <QSettings>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QXmlStreamWriter>
#include <QCoreApplication>
#include <QCryptographicHash>

#define PARAMAPSIZE 9

///
/// @brief 扫描排序类型结构体
/// @details 定义扫描和排序的配置参数
///
struct ScanSortTypes {
    int nScanHatchingFirst = 1;  // 优先扫描填充
    int nScanSortByPart = 1;     // 按零件排序
    int nWindDirection = 0;       // 风向设置
};

///
/// @brief XML元数据零件信息结构体
/// @details 存储零件相关的文件信息
///
struct MetaXMLPart {
    QString strJsonFile;    // JSON文件名
    QString strPartName;    // 零件名称

    MetaXMLPart() = default;
    
    /// @brief 构造函数
    /// @param jsonName JSON文件名
    /// @param partName 零件名称
    MetaXMLPart(const QString &jsonName, const QString &partName) {
        strJsonFile = jsonName;
        strPartName = partName;
    }
};

///
/// @brief 参数配置结构体
/// @details 存储速度和功率参数
///
struct Parameter {
    int nID = 0;           // 参数ID
    float fSpeed = 100;    // 速度值
    float fPower = 150;    // 功率值

    Parameter() = default;

    /// @brief 构造函数
    /// @param speed 速度值
    /// @param power 功率值
    Parameter(float speed, float power) {
        fSpeed = speed;
        fPower = power;
    }
};


inline QDebug operator<<(QDebug debug, const Parameter &p)
{
    debug << QString("{%1, %2, %3}").arg(p.nID).arg(p.fPower).arg(p.fSpeed).toLocal8Bit().constData();
    return debug;
}

///
/// @brief 数据块信息结构体
/// @details 存储数据块的位置、扫描参数和边界信息
///
struct BlockInfo {
    qint64 nPos = 0;               // 数据块位置
    int ScannerIndexRef = 0;       // 扫描器索引
    int VectorTypeRef = 0;         // 矢量类型

    // 边界框坐标
    float fMinX = FLT_MAX;         // X最小值
    float fMaxX = FLT_MIN;         // X最大值 
    float fMinY = FLT_MAX;         // Y最小值
    float fMaxY = FLT_MIN;         // Y最大值

    // 中心点坐标
    float fCentX = 0.0;           // X中心
    float fCentY = 0.0;           // Y中心

    BlockInfo() = default;         // 默认构造函数

    /// @brief 拷贝构造函数
    /// @param block 源数据块
    BlockInfo(const BlockInfo &block) {
        *this = block;
    }
};

///
/// @brief 文件块映射结构体
/// @details 存储文件ID和对应的数据块信息
///
struct FileBlockMap {
    QString strFileID;        // 文件ID
    BlockInfoPtr block;       // 数据块指针

    /// @brief 构造函数
    /// @param id 文件ID
    /// @param b 数据块指针
    FileBlockMap(const QString &id, const BlockInfoPtr &b) {
        strFileID = id;
        block = b;
    }
};


///
/// @brief 解析JSON数据为对象
/// @param data JSON字节数组
/// @param successful 可选的解析结果标志
/// @return 解析得到的JSON对象
/// @details 实现步骤:
///   1. 解析JSON数据
///   2. 检查解析结果
///   3. 返回JSON对象
///
QJsonObject getObject(const QByteArray &data, bool *successful = nullptr)
{
    // 解析JSON数据
    QJsonParseError error;
    auto documnet = QJsonDocument::fromJson(data, &error);

    // 设置解析结果标志
    if(successful) *successful = (QJsonParseError::NoError == error.error);

    // 返回解析结果
    if(QJsonParseError::NoError == error.error) return documnet.object();

    // 解析失败返回空对象
    return QJsonObject();
}

QList<FileBlockMap> sortBlock(const ScanSortTypes &, const int &, const QStringList &, const QList<BinFile> &);

SLJobFileWriter::SLJobFileWriter()
{
    fileWriter = new WriteJFile();
    buffPara = new PARAWRITEBUFF();
    algorithm = new AlgorithmApplication();
}

///
/// @brief SLJob文件写入器析构函数
/// @details 实现步骤:
///   1. 等待写入线程完成
///   2. 释放写入器资源
///   3. 释放缓冲区
///   4. 释放算法对象
///
SLJobFileWriter::~SLJobFileWriter()
{
    // 等待写入线程完成并释放
    if(fileWriter)
    {
        fileWriter->wait();
        delete fileWriter;
        fileWriter = nullptr;
    }

    // 释放缓冲区
    if(buffPara)
    {
        delete buffPara;
        buffPara = nullptr;
    }

    // 释放算法对象
    if(algorithm)
    {
        delete algorithm;
        algorithm = nullptr;
    }
}


///
/// @brief 初始化文件写入器
/// @return 初始化结果,true表示成功,false表示失败
/// @details 实现步骤:
///   1. 设置算法缓冲区参数
///   2. 设置文件写入器缓冲区
///   3. 创建文件数据
///   4. 写入文件头信息
///
bool SLJobFileWriter::initFileWrite()
{
    // 设置算法缓冲区参数
    algorithm->setBuffParas(buffPara, fileWriter->m_nScannerIndex);

    // 设置文件写入器缓冲区
    fileWriter->setParaWriteBuff(buffPara);

    // 创建文件数据
    bool bResult = buffPara->createUFileData();

    // 写入文件头信息
    SLJFileWriter::writeFileHeaderInfo(&buffPara->gFile);

    return bResult;
}


///
/// @brief 启动缓冲区写入
/// @details 实现步骤:
///   1. 设置文件写入状态为开始
///   2. 启动写入线程
///
void SLJobFileWriter::startBuffWriter()
{
    // 设置写入状态为开始
    fileWriter->setUFFStatus(UFFWRITE_BEGIN);

    // 启动写入线程
    fileWriter->start();
}

void SLJobFileWriter::waitBuffWriter()
{
    fileWriter->setUFFStatus(UFFWRITE_END);
    fileWriter->wait();
    if(curLayerInfo)
    {
        qint64 nCurPos = buffPara->gFile.pos();
        buffPara->gFile.seek(curLayerInfo->nPos);
        SLJFileWriter::writeFileValue(&buffPara->gFile, curLayerInfo->nLayerSz);
        buffPara->gFile.seek(nCurPos);

        //        QString str;
        //        QDebug debug(&str);
        //        debug << "[" <<curLayerInfo->blockList.size() << "]";
        //        for(const auto &block : curLayerInfo->blockList)
        //        {
        //            debug << block.paraRefer.nVectorTypeID << block.binFileInfo.nPos;
        //        }
        //        qDebug() << "waitBuffWriter" << str;
    }
}

void SLJobFileWriter::setScannerIndex(const int &index)
{
    if(fileWriter) fileWriter->m_nScannerIndex = index;
}

void SLJobFileWriter::writeLayerInfo(const int &nHei, const BOUNDINGRECT &, const double &totalArea)
{
    curLayerInfo = new JFileLayerInfo;
    curLayerInfo->nPos = buffPara->gFile.pos() + 4;
    curLayerInfo->_totalArea = totalArea;
    fileWriter->setJFileLayerInfo(curLayerInfo);
    layerInfoPtrMap.insert(nHei, JFileLayerInfoPtr(curLayerInfo));
    SLJFileWriter::writeLayerInfo(&buffPara->gFile, curLayerInfo, totalArea);
}

///
/// @brief 创建USP文件及其相关JSON信息文件
/// @param thickness 层厚
/// @param boundingBox 边界框
/// @param minHei 最小高度
/// @param maxHei 最大高度
/// @details 实现步骤:
///   1. 更新文件头信息
///   2. 创建临时JSON文件存储层信息
///   3. 遍历所有层信息,写入每层的扫描块数据
///   4. 计算文件整体边界信息
///   5. 合并生成最终的LBF文件
///
void SLJobFileWriter::createUSPFile(const QString &, const QString &, const float &thickness,
                                   BOUNDINGBOX *boundingBox, const int &minHei, const int &maxHei, QJsonObject &)
{
    // 更新文件头信息中的数据大小
    if(curLayerInfo)
    {
        qint64 nCurPos = buffPara->gFile.pos();
        buffPara->gFile.seek(57);
        SLJFileWriter::writeFileValue(&buffPara->gFile, quint32(nCurPos - 1 - 61));
        buffPara->gFile.seek(nCurPos);
    }

    // 创建临时JSON文件
    QString strBinFileName = buffPara->gFile.fileName();
    QString strJsonFileName = strBinFileName.left(strBinFileName.size() - 3) + "lbftt";
    QFile file(strJsonFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        file.resize(0);
        QJsonObject obj;

        // 初始化文件边界值
        float fFileMinX = FLT_MAX;
        float fFileMinY = FLT_MAX;
        float fFileMaxX = FLT_MIN;
        float fFileMaxY = FLT_MIN;

        // 遍历每层信息写入JSON
        for(auto i = layerInfoPtrMap.begin(); i != layerInfoPtrMap.end(); ++ i)
        {
            // 构建层信息对象
            QJsonObject layerObj;
            layerObj.insert("Z", i.key());
            
            // 写入层内扫描块信息
            QJsonArray blockArray;
            for(const auto &blockPtr : i.value()->blockList)
            {
                QJsonObject blockObj;
                blockObj.insert("Pos", blockPtr->binFileInfo.nPos);
                blockObj.insert("ScannerIndexRef", blockPtr->paraRefer.nScannerIndex);
                blockObj.insert("VectorTypeRef", blockPtr->paraRefer.nVectorTypeID);
                blockObj.insert("fMinX", blockPtr->fMinX);
                blockObj.insert("fMinY", blockPtr->fMinY);
                blockObj.insert("fMaxX", blockPtr->fMaxX);
                blockObj.insert("fMaxY", blockPtr->fMaxY);
                blockObj.insert("fCentX", (blockPtr->fMinX + blockPtr->fMaxX) * 0.5);
                blockObj.insert("fCentY", (blockPtr->fMinY + blockPtr->fMaxY) * 0.5);
                blockArray.append(blockObj);
            }

            // 写入层整体信息
            layerObj.insert("fTotalArea", i.value()->_totalArea);
            layerObj.insert("fMinX", i.value()->fMinX);
            layerObj.insert("fMaxX", i.value()->fMaxX);
            layerObj.insert("fMinY", i.value()->fMinY);
            layerObj.insert("fMaxY", i.value()->fMaxY);
            layerObj.insert("fCenterX", (i.value()->fMinX + i.value()->fMaxX) * 0.5);
            layerObj.insert("fCenterY", (i.value()->fMinY + i.value()->fMaxY) * 0.5);
            layerObj.insert("Blocks", blockArray);

            // 写入层数据
            file.write(QJsonDocument(layerObj).toJson(QJsonDocument::Compact) + "\n");

            // 更新文件边界值
            fFileMinX = qMin(fFileMinX, i.value()->fMinX);
            fFileMaxX = qMax(fFileMaxX, i.value()->fMaxX);
            fFileMinY = qMin(fFileMinY, i.value()->fMinY);
            fFileMaxY = qMax(fFileMaxY, i.value()->fMaxY);
        }

        // 构建文件整体信息
        obj.insert("FileName", QFileInfo(strBinFileName).completeBaseName() + ".bin");
        obj.insert("Xmin", fFileMinX);
        obj.insert("Ymin", fFileMinY);
        obj.insert("Zmin", minHei * UNITFACTOR);
        obj.insert("Xmax", fFileMaxX);
        obj.insert("Ymax", fFileMaxY);
        obj.insert("Zmax", maxHei * UNITFACTOR);
        obj.insert("FileSize", buffPara->gFile.size());
        obj.insert("LayerThickness", double(thickness));
        obj.insert("Zmin", minHei * UNITFACTOR);
        obj.insert("Zmax", maxHei * UNITFACTOR);

        // 生成最终LBF文件
        QFile nFile(strBinFileName.left(strBinFileName.size() - 3) + "lbf");
        if(nFile.open(QIODevice::ReadWrite)) {
            file.seek(0);

            // 写入文件信息和层数据
            nFile.write(QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n");
            while(!file.atEnd())
            {
                nFile.write(file.readLine());
            }
            nFile.close();
        }
        file.close();
        file.remove();
    }
}


///
/// @brief 创建作业文件内容XML
/// @param path 文件路径
/// @param listPartName 零件名称列表
/// @param saveName 保存名称
/// @param writerBuffer 写入缓冲区参数
/// @details 实现步骤:
///   1. 创建元数据XML文件
///   2. 创建并打开Content.xml文件
///   3. 写入XML头信息和版本
///   4. 写入加密策略信息
///   5. 写入容器文件列表(元数据和二进制文件)
///
void SLJobFileWriter::createJFileContentXml(const QString &path, const QStringList &listPartName, 
                                          const QString &saveName,
                                          const QSharedPointer<WriterBufferParas> &writerBuffer)
{
    // 创建元数据XML文件
    createJFileMetaDataXML(path, listPartName, saveName, writerBuffer);

    // 创建Content.xml文件
    QFile file(path + "Content.xml");
    if(file.open(QIODevice::ReadWrite))
    {
        file.resize(0);
        QXmlStreamWriter writer(&file);
        writer.setCodec("UTF-8");
        writer.writeStartDocument();

        // 写入根元素和命名空间
        writer.writeStartElement("ContainerContent");
        writer.writeAttribute("xmlns", "http://schemas.materialise.com/AM/MatJob/Content_1");

        // 写入版本信息
        writer.writeStartElement("Version");
        writer.writeAttribute("Major", "2");
        writer.writeAttribute("Minor", "0");
        writer.writeAttribute("Revision", "0");
        writer.writeEndElement();

        // 写入加密策略
        writer.writeStartElement("EncryptionStrategies");
        writer.writeStartElement("EncryptionStrategy");
        writer.writeAttribute("Id", "none");
        writer.writeAttribute("Method", "none");
        writer.writeEndElement();
        writer.writeEndElement();

        // 写入容器文件列表
        writer.writeStartElement("ContainerFiles");
        
        // 写入元数据文件信息
        writer.writeStartElement("MetadataFile");
        writer.writeAttribute("FileName", "JobMetaData.job");
        writer.writeAttribute("Md5", getFileHash(path + "JobMetaData.job"));
        writer.writeAttribute("EncryptionStrategyRef", "none");
        writer.writeEndElement();

        // 写入所有零件二进制文件信息
        for(const auto &name : listPartName)
        {
            addBinFileToContentXML(&writer, path, name + ".bin");
        }

        writer.writeEndElement();
        writer.writeEndElement();
        writer.writeEndDocument();

        file.close();
    }
}

///
/// @brief 创建作业文件元数据XML
/// @param path 文件路径
/// @param listPartName 零件名称列表
/// @param saveName 保存名称
/// @param writerBuffer 写入缓冲区参数
/// @details 实现步骤:
///   1. 获取参数配置(扫描排序、支撑参数等)
///   2. 创建参数映射表
///   3. 创建并打开JobMetaData.job文件
///   4. 写入文件头信息(版本、应用等)
///   5. 写入作业基本信息(名称、尺寸等)
///   6. 写入机器类型信息
///   7. 写入零件信息
///   8. 写入矢量类型定义
///   9. 写入工艺参数集
///   10. 写入二进制文件信息
///   11. 写入层信息
///
void SLJobFileWriter::createJFileMetaDataXML(const QString &path, const QStringList &listPartName, const QString &saveName,
                                             const QSharedPointer<WriterBufferParas> &writerBuffer)
{
    // 获取参数配置
    const BPCUSTOMERPARAS &bpcParas = *writerBuffer->_bpcParaPtr.data();
    const BPPBUILDPARAMETERS &bppParas = *writerBuffer->_bppParaPtr.data();
    const JsonParsingPtr &extendedParas = writerBuffer->_extendedParaPtr;

    // 配置扫描排序参数
    ScanSortTypes sortParas;
    sortParas.nScanHatchingFirst = extendedParas->getValue<int>("SLMSortTypes/nScanHatchingFirst", sortParas.nScanHatchingFirst);
    sortParas.nScanSortByPart = extendedParas->getValue<int>("SLMSortTypes/nScanSortByPart", sortParas.nScanSortByPart);
    sortParas.nWindDirection = bpcParas.mExtendParasObj["WindDirection"].toInt();

    // 获取支撑参数
    int nSolidSupportHatchingPower = extendedParas->getValue<int>("SolidSupportHatching/nPower", 80);
    int nSolidSupportHatchingSpeed = extendedParas->getValue<int>("SolidSupportHatching/nMarkSpeed", 1000);
    int nSolidSupportBorderPower = extendedParas->getValue<int>("SolidSupportBorder/nPower", 80);
    int nSolidSupportBorderSpeed = extendedParas->getValue<int>("SolidSupportBorder/nMarkSpeed", 1000);

//    if(extendedParas->getExtendedValue<int>("Global/nExportFileMode", 0))
//    {
//        reader.setValue("SLMSortTypes/nScanHatchingFirst", sortParas.nScanHatchingFirst);
//        reader.setValue("SLMSortTypes/nScanSortByPart", sortParas.nScanSortByPart);

//        reader.setValue("SolidSupportHatching/nPower", nSolidSupportHatchingPower);
//        reader.setValue("SolidSupportHatching/nMarkSpeed", nSolidSupportHatchingSpeed);
//        auto nHatchingType = reader.value("SolidSupportHatching/nHatchingType", 0).toInt();
//        auto fLineSpacing = reader.value("SolidSupportHatching/fLineSpacing", 0.01).toDouble();
//        auto fAngle = reader.value("SolidSupportHatching/fAngle", 60.3).toDouble();
//        reader.setValue("SolidSupportHatching/nHatchingType", nHatchingType);
//        reader.setValue("SolidSupportHatching/fLineSpacing", fLineSpacing);
//        reader.setValue("SolidSupportHatching/fAngle", fAngle);

//        reader.setValue("SolidSupportBorder/nPower", nSolidSupportBorderPower);
//        reader.setValue("SolidSupportBorder/nMarkSpeed", nSolidSupportBorderSpeed);
//    }

    // 创建参数映射表
    QMap<int, Parameter> parasMap;
    parasMap.insert(0, Parameter(bppParas.sBorderPara.nMarkSpeed[0],
                                 bppParas.sGeneralPara.nLaserPower[bppParas.sBorderPara.nIndex_Power[0]]));
    parasMap.insert(1, Parameter(bppParas.sHatchingPara.nMarkSpeed[0],
                                 bppParas.sGeneralPara.nLaserPower[bppParas.sHatchingPara.nIndex_Power[0]]));
    parasMap.insert(2, Parameter(bppParas.sBorderPara.nMarkSpeed[0],
                                 bppParas.sGeneralPara.nLaserPower[bppParas.sBorderPara.nIndex_Power[0]]));
    parasMap.insert(3, Parameter(bppParas.sSurfacePara_Up.nMarkSpeed[0],
                                 bppParas.sGeneralPara.nLaserPower[bppParas.sSurfacePara_Up.nIndex_Power[0]]));
    parasMap.insert(4, Parameter(bppParas.sBorderPara.nMarkSpeed[0],
                                 bppParas.sGeneralPara.nLaserPower[bppParas.sBorderPara.nIndex_Power[0]]));
    parasMap.insert(5, Parameter(bppParas.sSurfacePara_Dw.nMarkSpeed[0],
                                 bppParas.sGeneralPara.nLaserPower[bppParas.sSurfacePara_Dw.nIndex_Power[0]]));
    parasMap.insert(6, Parameter(bppParas.sSupportPara.nMarkSpeed,
                                 bppParas.sGeneralPara.nLaserPower[bppParas.sSupportPara.nIndex_Power]));
    parasMap.insert(7, Parameter(nSolidSupportBorderSpeed, nSolidSupportBorderPower));
    parasMap.insert(8, Parameter(nSolidSupportHatchingSpeed, nSolidSupportHatchingPower));
    
    // 创建并写入XML文件
    QFile file(path + "JobMetaData.job");
    if(file.open(QIODevice::ReadWrite))
    {
        file.resize(0);
        QXmlStreamWriter writer(&file);

        writer.setCodec("UTF-8");
        //        writer.setAutoFormatting(true);
        // 写入文件头
        writer.writeStartDocument();
        writer.writeStartElement("BuildJob");
        writer.writeTextElement("JobID", "test-job-file");

        writer.writeStartElement("FileInfo");

        writer.writeStartElement("Version");
        writer.writeTextElement("Major", "2");
        writer.writeTextElement("Minor", "1");
        writer.writeEndElement();

        writer.writeStartElement("WrittenBy");
        writer.writeStartElement("Application");
        writer.writeAttribute("Name", "Materialise Build Processor for LM");
        writer.writeAttribute("Version", "5.0.10.0");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeEndElement();
        writer.writeStartElement("GeneralInfo");
        writer.writeTextElement("JobName", saveName);

        double fXmin = FLT_MAX;
        double fYmin = FLT_MAX;
        double fZmin = FLT_MAX;
        double fXmax = FLT_MIN;
        double fYmax = FLT_MIN;
        double fZmax = FLT_MIN;
        for(const auto &part : listPartName)
        {
            QFile file(path + part + ".lbf");
            if(file.open(QIODevice::ReadOnly))
            {
                auto obj = getObject(file.readLine());
                fXmin = qMin(fXmin, obj["Xmin"].toDouble());
                fYmin = qMin(fYmin, obj["Ymin"].toDouble());
                fZmin = qMin(fZmin, obj["Zmin"].toDouble());

                fXmax = qMax(fXmax, obj["Xmax"].toDouble());
                fYmax = qMax(fYmax, obj["Ymax"].toDouble());
                fZmax = qMax(fZmax, obj["Zmax"].toDouble());
                file.close();
            }
        }

        writer.writeStartElement("JobDimensions");
        writer.writeTextElement("Xmin", Num2String::text(fXmin, 3));
        writer.writeTextElement("Ymin", Num2String::text(fYmin, 3));
        writer.writeTextElement("Zmin", Num2String::text(fZmin, 3));
        writer.writeTextElement("Xmax", Num2String::text(fXmax, 3));
        writer.writeTextElement("Ymax", Num2String::text(fYmax, 3));
        writer.writeTextElement("Zmax", Num2String::text(fZmax, 3));
        writer.writeEndElement();

        writer.writeTextElement("Material", "CX");

        writer.writeStartElement("Properties");
        writer.writeStartElement("Property");
        writer.writeAttribute("Name", "SplitFieldsNumber");
        writer.writeAttribute("Type", "Integer");
        writer.writeTextElement("Value", "1");
        writer.writeEndElement();
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeStartElement("MachineType");
        writer.writeStartElement("PhysicalProperties");
        writer.writeStartElement("BuildVolume");

        writer.writeStartElement("BuildDimension");
        writer.writeStartElement("Shape");
        writer.writeStartElement("Box");
        writer.writeStartElement("Dimensions");
        writer.writeAttribute("X", Num2String::text(bpcParas.nPlatWidthX, 4));
        writer.writeAttribute("Y", Num2String::text(bpcParas.nPlatWidthY, 4));
        writer.writeAttribute("Z", "310.0000");
        writer.writeEndElement();
        writer.writeEndElement();
        writer.writeEndElement();
        writer.writeStartElement("Translation");
        writer.writeAttribute("X", "0.0000");
        writer.writeAttribute("Y", "0.0000");
        writer.writeAttribute("Z", "0.0000");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeStartElement("NoBuildDimensions");
        writer.writeStartElement("NoBuildDimension");
        writer.writeStartElement("Shape");
        writer.writeStartElement("Box");
        writer.writeStartElement("Dimensions");
        writer.writeAttribute("X", "262.0000");
        writer.writeAttribute("Y", "255.0000");
        writer.writeAttribute("Z", "2.0000");
        writer.writeEndElement();
        writer.writeEndElement();
        writer.writeEndElement();
        writer.writeStartElement("Translation");
        writer.writeAttribute("X", "0.0000");
        writer.writeAttribute("Y", "3.5000");
        writer.writeAttribute("Z", "0.0000");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeEndElement();
        writer.writeEndElement();
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeStartElement("Parts");
        int nPartIndex = 0;

        for(const auto &part : listPartName)
        {
            QFile file(path + part + ".lbf");
            if(file.open(QIODevice::ReadOnly))
            {
                auto obj = getObject(file.readLine());
                addPartToMetaDataXML(&writer, nPartIndex, part, obj["Xmin"].toDouble(), obj["Ymin"].toDouble()
                                     , obj["Zmin"].toDouble(), obj["Xmax"].toDouble(), obj["Ymax"].toDouble(), obj["Zmax"].toDouble());
                file.close();
            }
            ++ nPartIndex;
        }

        writer.writeEndElement();

        writer.writeStartElement("VectorTypes");

        writer.writeStartElement("VectorType");
        writer.writeAttribute("Id", "0");
        writer.writeTextElement("Name", "In Skin Border");
        writer.writeStartElement("Flags");
        writer.writeAttribute("Border", "1");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeStartElement("VectorType");
        writer.writeAttribute("Id", "1");
        writer.writeTextElement("Name", "In Skin Hatching");
        writer.writeStartElement("Flags");
        writer.writeAttribute("Hatching", "1");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeStartElement("VectorType");
        writer.writeAttribute("Id", "2");
        writer.writeTextElement("Name", "Up Skin Border");
        writer.writeStartElement("Flags");
        writer.writeAttribute("Upfacing", "1");
        writer.writeAttribute("Border", "1");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeStartElement("VectorType");
        writer.writeAttribute("Id", "3");
        writer.writeTextElement("Name", "Up Skin Hatching");
        writer.writeStartElement("Flags");
        writer.writeAttribute("Upfacing", "1");
        writer.writeAttribute("Hatching", "1");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeStartElement("VectorType");
        writer.writeAttribute("Id", "4");
        writer.writeTextElement("Name", "Down Skin Border");
        writer.writeStartElement("Flags");
        writer.writeAttribute("Downfacing", "1");
        writer.writeAttribute("Border", "1");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeStartElement("VectorType");
        writer.writeAttribute("Id", "5");
        writer.writeTextElement("Name", "Down Skin Hatching");
        writer.writeStartElement("Flags");
        writer.writeAttribute("Downfacing", "1");
        writer.writeAttribute("Hatching", "1");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeStartElement("VectorType");
        writer.writeAttribute("Id", "6");
        writer.writeTextElement("Name", "Non-Solid Support");
        writer.writeStartElement("Flags");
        writer.writeAttribute("ZeroVolume", "1");
        writer.writeAttribute("Support", "1");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeStartElement("VectorType");
        writer.writeAttribute("Id", "7");
        writer.writeTextElement("Name", "Solid Support Border");
        writer.writeStartElement("Flags");
        writer.writeAttribute("Support", "1");
        writer.writeAttribute("Border", "1");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeStartElement("VectorType");
        writer.writeAttribute("Id", "8");
        writer.writeTextElement("Name", "Solid Support Hatching");
        writer.writeStartElement("Flags");
        writer.writeAttribute("Support", "1");
        writer.writeAttribute("Hatching", "1");
        writer.writeEndElement();
        writer.writeEndElement();

        writer.writeEndElement();

        writer.writeStartElement("ProcessParameterSets");
        const auto keys = parasMap.keys();

        for(int iScanner = 0; iScanner < bpcParas.nScannerNumber; ++ iScanner)
        {
            for(const auto &key : keys)
            {
                addParemeterToMetaDataXML(&writer, iScanner, iScanner * PARAMAPSIZE + key, parasMap[key].fSpeed, 0.065f, parasMap[key].fPower);
            }
        }
        writer.writeEndElement();

        int nIndex = 0;
        writer.writeStartElement("BinaryFiles");
        for(const auto &name : listPartName)
        {
            addBinFileToMetaDataXML(&writer, path, nIndex ++, name + ".bin", 0);
        }
        writer.writeEndElement();

        int nMinLayer = 0x7FFFFFFF;
        int nMaxLayer = 0;
        double thickness = 1;
        QList<BinFile> binFileList;
        loadJsonFiles(path, listPartName, binFileList, nMinLayer, nMaxLayer, thickness);
        thickness = qRound(thickness * 1E6 + 0.1) * 1E-6;
        // 写入层信息
        writer.writeStartElement("Layers");
        writer.writeTextElement("LayerThickness", Num2String::text(thickness, 4));
        for(int iLayer = nMinLayer; iLayer <= nMaxLayer; iLayer += 1)
        {
            bool bValidLayer = false;
            float fMinX = FLT_MAX;
            float fMaxX = FLT_MIN;
            float fMinY = FLT_MAX;
            float fMaxY = FLT_MIN;
            double fTotalArea = 0.0;
            for(const auto &binFile : qAsConst(binFileList))
            {
                if(binFile.layerMap.contains(iLayer))
                {
                    bValidLayer = true;
                    const auto &layerInfo = binFile.layerMap[iLayer];
                    fMinX = qMin(fMinX, layerInfo.fMinX);
                    fMaxX = qMax(fMaxX, layerInfo.fMaxX);
                    fMinY = qMin(fMinY, layerInfo.fMinY);
                    fMaxY = qMax(fMaxY, layerInfo.fMaxY);
                    fTotalArea += layerInfo._totalArea;
                }
            }
            if(false == bValidLayer) continue;

            writer.writeStartElement("Layer");
            QString str;
            str.sprintf("%0.4f", iLayer * UNITFACTOR);
            writer.writeTextElement("Z", str);
            writer.writeTextElement("LayerScanTime", "1.000");
            writer.writeTextElement("LayerArea", Num2String::text(fTotalArea, 2));
            writer.writeStartElement("Summary");
            writer.writeTextElement("TotalMarkDistance", "1.000");
            writer.writeTextElement("TotalJumpDistance", "1.000");
            writer.writeTextElement("XMin", Num2String::text(fMinX, 3));
            writer.writeTextElement("XMax", Num2String::text(fMaxX, 3));
            writer.writeTextElement("YMin", Num2String::text(fMinY, 3));
            writer.writeTextElement("YMax", Num2String::text(fMaxY, 3));
            writer.writeEndElement();

            writer.writeStartElement("LayerRegions");
            writer.writeStartElement("LayerRegion");
            writer.writeAttribute("Id", "-1");
            writer.writeTextElement("ScanTime", "1.0000");
            writer.writeStartElement("RegionSummary");
            writer.writeTextElement("TotalMarkDistance", "1.000");
            writer.writeTextElement("TotalJumpDistance", "1.000");
            writer.writeTextElement("XMin", Num2String::text(fMinX, 3));
            writer.writeTextElement("XMax", Num2String::text(fMaxX, 3));
            writer.writeTextElement("YMin", Num2String::text(fMinY, 3));
            writer.writeTextElement("YMax", Num2String::text(fMaxY, 3));
            writer.writeEndElement();
            writer.writeEndElement();

            writer.writeEndElement();

            QList<FileBlockMap> listFileBlockMap = sortBlock(sortParas, iLayer, listPartName, binFileList);

            for(const auto &fileBlock : qAsConst(listFileBlockMap))
            {
                addBlockToMetaDataXML(&writer, fileBlock.strFileID, fileBlock.block);
            }

            writer.writeEndElement();
        }
        writer.writeEndElement();


        writer.writeEndElement();
        writer.writeEndDocument();
        file.close();
    }
}

///
/// @brief 向Content.xml添加二进制文件信息
/// @param writer XML写入器
/// @param fileFolder 文件夹路径
/// @param fileName 文件名
/// @details 实现步骤:
///   1. 写入二进制文件标记
///   2. 写入文件名
///   3. 写入加密策略(none)
///   4. 写入容器标志(外部)
///   5. 写入文件MD5校验值
///
void SLJobFileWriter::addBinFileToContentXML(QXmlStreamWriter *writer, const QString &fileFolder, const QString &fileName)
{
    // 写入二进制文件标记和属性
    writer->writeStartElement("BinaryFile");
    writer->writeAttribute("FileName", fileName);
    writer->writeAttribute("EncryptionStrategyRef", "none");
    writer->writeAttribute("IsOutsideContainer", "true");
    writer->writeAttribute("Md5", getFileHash(fileFolder + fileName));
    writer->writeEndElement();
}


///
/// @brief 向XML添加激光参数信息
/// @param writer XML写入器
/// @param scannerIndex 扫描器索引
/// @param index 参数索引
/// @param speed 扫描速度
/// @param diameter 激光直径
/// @param power 激光功率
/// @details 实现步骤:
///   1. 写入参数集标记和扫描场信息
///   2. 写入参数ID和名称
///   3. 写入扫描速度
///   4. 写入激光器参数(直径和功率)
///
void SLJobFileWriter::addParemeterToMetaDataXML(QXmlStreamWriter *writer, const int &scannerIndex, 
                                               const int &index, const float &speed,
                                               const float &diameter, const float &power)
{
    // 写入参数集标记和扫描场信息
    writer->writeStartElement("ParameterSet");
    writer->writeAttribute("ScanField", QString::number(scannerIndex));

    // 写入参数ID和名称
    writer->writeTextElement("ID", Num2String::text(index));
    writer->writeTextElement("Name", "BuildStrategy_UM300-T50_BuildInstance_0_ScanField_0");

    // 写入扫描速度
    writer->writeTextElement("LaserSpeed", Num2String::text(speed, 4));

    // 写入激光器参数
    writer->writeStartElement("LaserSet");
    writer->writeTextElement("ID", "0");
    writer->writeTextElement("LaserDiameter", Num2String::text(diameter, 4));
    writer->writeTextElement("LaserPower", Num2String::text(power, 4));
    writer->writeEndElement();
    writer->writeEndElement();
}


///
/// @brief 向XML添加二进制文件信息
/// @param writer XML写入器
/// @param path 文件路径
/// @param index 文件索引
/// @param fileName 文件名
/// @param sz 文件大小(未使用)
/// @details 实现步骤:
///   1. 写入二进制文件标记
///   2. 写入文件ID和名称
///   3. 写入文件校验和大小信息
///
void SLJobFileWriter::addBinFileToMetaDataXML(QXmlStreamWriter *writer, const QString &path, 
                                             const int &index, const QString &fileName, const qint64 &sz)
{
    // 写入二进制文件标记
    writer->writeStartElement("BinaryFile");

    // 写入文件ID和名称
    writer->writeTextElement("ID", Num2String::text(index));
    writer->writeTextElement("Name", fileName);

    // 写入文件校验和大小信息
    writer->writeStartElement("CRC");
    QFile file(path + fileName);
    writer->writeTextElement("FileSize", Num2String::text(file.size()));
    writer->writeEndElement();
    writer->writeEndElement();
}


///
/// @brief 向XML添加零件信息
/// @param writer XML写入器
/// @param index 零件索引
/// @param partName 零件名称
/// @param xmin X最小坐标
/// @param ymin Y最小坐标
/// @param zmin Z最小坐标
/// @param xmax X最大坐标
/// @param ymax Y最大坐标
/// @param zmax Z最大坐标
/// @details 实现步骤:
///   1. 写入零件基本信息(ID和名称)
///   2. 写入零件尺寸信息(边界框坐标)
///   3. 写入零件属性信息(设备配置)
///
void SLJobFileWriter::addPartToMetaDataXML(QXmlStreamWriter *writer, const int &index, const QString &partName, 
                                          const float &xmin, const float &ymin, const float &zmin, 
                                          const float &xmax, const float &ymax, const float &zmax)
{
    // 写入零件基本信息
    writer->writeStartElement("Part");
    writer->writeTextElement("ID", Num2String::text(index));
    writer->writeTextElement("Name", partName);

    // 写入零件尺寸信息
    writer->writeStartElement("Dimensions");
    writer->writeTextElement("Xmin", Num2String::text(xmin, 3));
    writer->writeTextElement("Ymin", Num2String::text(ymin, 3));
    writer->writeTextElement("Zmin", Num2String::text(zmin, 3));
    writer->writeTextElement("Xmax", Num2String::text(xmax, 3));
    writer->writeTextElement("Ymax", Num2String::text(ymax, 3));
    writer->writeTextElement("Zmax", Num2String::text(zmax, 3));
    writer->writeEndElement();

    // 写入零件属性信息
    writer->writeStartElement("Properties");
    writer->writeStartElement("Property");
    writer->writeAttribute("Name", "ProfileName");
    writer->writeAttribute("Type", "String");
    writer->writeTextElement("Value", "UM300-T50");
    writer->writeEndElement();
    writer->writeEndElement();
    writer->writeEndElement();
}


///
/// @brief 向XML添加数据块信息
/// @param writer XML写入器
/// @param fileID 文件ID
/// @param block 数据块信息指针
/// @details 实现步骤:
///   1. 写入数据块开始标记
///   2. 写入引用信息(零件、工艺和矢量类型)
///   3. 写入摘要信息(标记距离、跳转距离等)
///   4. 写入二进制数据位置信息
///
void SLJobFileWriter::addBlockToMetaDataXML(QXmlStreamWriter *writer, const QString &fileID, const BlockInfoPtr &block)
{
    // 写入数据块标记
    writer->writeStartElement("DataBlock");

    // 写入引用信息
    writer->writeStartElement("References");
    writer->writeAttribute("Part", fileID);
    const QString strVectorType = Num2String::text(getVectorType(block->VectorTypeRef));
    writer->writeAttribute("Process", Num2String::text(block->ScannerIndexRef * PARAMAPSIZE + getVectorType(block->VectorTypeRef)));
    writer->writeAttribute("VectorTypeRef", strVectorType);
    writer->writeEndElement();

    // 写入摘要信息
    writer->writeStartElement("Summary"); 
    writer->writeAttribute("MarkDistance", Num2String::text(1.0, 4));
    writer->writeAttribute("JumpDistance", Num2String::text(1.0, 4));
    writer->writeAttribute("NumMarkSegments", "1");
    writer->writeAttribute("NumJumpSegments", "1");
    writer->writeEndElement();

    // 写入二进制数据位置
    writer->writeStartElement("Bin");
    writer->writeAttribute("FileID", fileID);
    writer->writeAttribute("Pos", Num2String::text(block->nPos));
    writer->writeEndElement();

    writer->writeEndElement();
}


///
/// @brief 加载JSON文件并解析层信息
/// @param path 文件路径
/// @param listPartName 零件名称列表
/// @param binFileList [out] 二进制文件信息列表
/// @param nMinLayer [out] 最小层号
/// @param nMaxLayer [out] 最大层号
/// @param thickness [out] 层厚
/// @details 实现步骤:
///   1. 清空输出列表
///   2. 遍历所有零件文件
///   3. 读取文件头获取层厚信息
///   4. 逐行解析每层数据
///   5. 解析层内扫描块信息
///   6. 更新层号范围
///
void SLJobFileWriter::loadJsonFiles(const QString &path, const QStringList &listPartName, 
                                  QList<BinFile> &binFileList,
                                  int &nMinLayer, int &nMaxLayer, double &thickness)
{
    // 清空输出列表
    binFileList.clear();

    // 遍历所有零件文件
    for(const auto &name : listPartName)
    {
        QFile file(path + name + ".lbf");
        if(file.open(QIODevice::ReadOnly))
        {
            BinFile binFile;

            // 读取文件头获取层厚
            auto obj = getObject(file.readLine());
            binFile.strFileName = name;
            thickness = qMin(thickness, qRound(obj["LayerThickness"].toDouble() * 1E6 + 0.1) * 1E-6);

            // 逐行解析层数据
            while(!file.atEnd())
            {
                Layer sLayer;
                bool bOk = false;
                auto layerObj = getObject(file.readLine(), &bOk);
                if(false == bOk) continue;

                // 解析层基本信息
                sLayer.Z = layerObj["Z"].toInt();
                sLayer._totalArea = layerObj["fTotalArea"].toDouble();
                sLayer.fMinX = layerObj["fMinX"].toDouble();
                sLayer.fMaxX = layerObj["fMaxX"].toDouble();
                sLayer.fMinY = layerObj["fMinY"].toDouble();
                sLayer.fMaxY = layerObj["fMaxY"].toDouble();
                sLayer.fCenterX = layerObj["fCenterX"].toDouble();
                sLayer.fCenterY = layerObj["fCenterY"].toDouble();

                // 解析层内扫描块信息
                const auto blockArray = layerObj["Blocks"].toArray();
                for (const auto &block : qAsConst(blockArray))
                {
                    BlockInfo sBlock;
                    auto blockObj = block.toObject();
                    sBlock.nPos = blockObj["Pos"].toVariant().toLongLong();
                    sBlock.ScannerIndexRef = blockObj["ScannerIndexRef"].toInt();
                    sBlock.VectorTypeRef = blockObj["VectorTypeRef"].toInt();
                    sBlock.fMinX = blockObj["fMinX"].toVariant().toFloat();
                    sBlock.fMinY = blockObj["fMinY"].toVariant().toFloat();
                    sBlock.fMaxX = blockObj["fMaxX"].toVariant().toFloat();
                    sBlock.fMaxY = blockObj["fMaxY"].toVariant().toFloat();
                    sBlock.fCentX = blockObj["fCentX"].toVariant().toFloat();
                    sBlock.fCentY = blockObj["fCentY"].toVariant().toFloat();
                    sLayer.blockList << BlockInfoPtr(new BlockInfo(sBlock));
                }
                binFile.layerMap.insert(sLayer.Z, sLayer);

                // 更新层号范围
                int nTempMin = binFile.layerMap.firstKey();
                int nTempMax = binFile.layerMap.lastKey();
                nMinLayer = nMinLayer < nTempMin ? nMinLayer : nTempMin;
                nMaxLayer = nMaxLayer > nTempMax ? nMaxLayer : nTempMax;
            }
            binFileList << binFile;
            file.close();
        }
        file.remove();
    }
}


///
/// @brief 获取矢量类型编号
/// @param type 输入的矢量类型标志
/// @return 对应的矢量类型编号
/// @details 实现步骤:
///   1. 支撑边界返回7
///   2. 支撑填充返回8
///   3. 零体积支撑返回6
///   4. 下表面返回5
///   5. 上表面返回3
///   6. 填充返回1
///   7. 边界返回0
///   8. 其他情况返回0
///
int SLJobFileWriter::getVectorType(const int &type)
{
    // 判断支撑相关类型
    if((VecSupport + VecBorder) == type) return 7;     // 支撑边界
    if((VecSupport + VecHatching) == type) return 8;   // 支撑填充
    if((VecSupport + VecZeroVolume) == type) return 6; // 零体积支撑

    // 判断表面类型
    if(VecDownfacing == type) return 5;  // 下表面
    if(VecUpfacing == type) return 3;    // 上表面

    // 判断基本类型
    if(VecHatching == type) return 1;    // 填充
    if(VecBorder == type) return 0;      // 边界

    return 0; // 默认返回边界类型
}

///
/// @brief 根据风向重新计算扫描块排序
/// @param fileBlockList 扫描块列表
/// @param windDirection 风向(0-3)
/// @details 实现步骤:
///   1. 根据风向选择排序方式:
///      - 0: Y坐标优先,从小到大排序
///      - 1: X坐标优先,从小到大排序
///      - 2: Y坐标优先,从大到小排序
///      - 3: X坐标优先,从大到小排序
///   2. 使用std::sort对扫描块进行排序
///   3. 排序权重计算:主坐标平方*10 + 次坐标
///
void recalcLayerout(QList<FileBlockMap> &fileBlockList, const int &windDirection) {
    switch(windDirection) {
    case 0:
    default:
        // Y坐标优先,从小到大排序
        std::sort(fileBlockList.begin(), fileBlockList.end(),
                  [](const FileBlockMap &fileBlock1, const FileBlockMap &fileBlock2) -> bool {
                      return (pow(fileBlock1.block->fCentY * 10, 2) + fileBlock1.block->fCentX <
                              pow(fileBlock2.block->fCentY * 10, 2) + fileBlock2.block->fCentX);
                  });
        break;
    case 1:
        // X坐标优先,从小到大排序
        std::sort(fileBlockList.begin(), fileBlockList.end(),
                  [](const FileBlockMap &fileBlock1, const FileBlockMap &fileBlock2) -> bool {
                      return (pow(fileBlock1.block->fCentX * 10, 2) + fileBlock1.block->fCentY <
                              pow(fileBlock2.block->fCentX * 10, 2) + fileBlock2.block->fCentY);
                  });
        break;
    case 2:
        // Y坐标优先,从大到小排序
        std::sort(fileBlockList.begin(), fileBlockList.end(),
                  [](const FileBlockMap &fileBlock1, const FileBlockMap &fileBlock2) -> bool {
                      return (pow(fileBlock1.block->fCentY * 10, 2) + fileBlock1.block->fCentX >
                              pow(fileBlock2.block->fCentY * 10, 2) + fileBlock2.block->fCentX);
                  });
        break;
    case 3:
        // X坐标优先,从大到小排序
        std::sort(fileBlockList.begin(), fileBlockList.end(),
                  [](const FileBlockMap &fileBlock1, const FileBlockMap &fileBlock2) -> bool {
                      return (pow(fileBlock1.block->fCentX * 10, 2) + fileBlock1.block->fCentY >
                              pow(fileBlock2.block->fCentX * 10, 2) + fileBlock2.block->fCentY);
                  });
        break;
    }
}


///
/// @brief 对整个文件的扫描块进行排序
/// @param sortParas 排序参数
/// @param layerHei 层高
/// @param listPartName 零件名称列表
/// @param binFileList 二进制文件列表
/// @return 排序后的文件块映射列表
/// @details 实现步骤:
///   1. 初始化三个分类列表(支撑、边界、填充)
///   2. 遍历所有文件的指定层数据块
///   3. 根据矢量类型将数据块分类
///   4. 对各分类列表按风向进行排序
///   5. 根据扫描优先级合并列表
///
QList<FileBlockMap> sortBlock_totalFile(const ScanSortTypes &sortParas, const int &layerHei, 
                                      const QStringList &listPartName, const QList<BinFile> &binFileList)
{
    QList<FileBlockMap> fileBlockList;

    // 初始化分类列表
    QList<FileBlockMap> supportList;
    QList<FileBlockMap> borderList; 
    QList<FileBlockMap> hatchingList;

    // 遍历文件分类数据块
    for(const auto &binFile : binFileList)
    {
        if(binFile.layerMap.contains(layerHei))
        {
            QString strFileID = Num2String::text(listPartName.indexOf(binFile.strFileName));
            for(const auto &block : binFile.layerMap[layerHei].blockList)
            {
                // 根据矢量类型分类
                if(VecSupport & block->VectorTypeRef) supportList << FileBlockMap(strFileID, block);
                else if(VecBorder & block->VectorTypeRef) borderList << FileBlockMap(strFileID, block);
                else hatchingList << FileBlockMap(strFileID, block);
            }
        }
    }

    // 对各类别进行风向排序
    recalcLayerout(supportList, sortParas.nWindDirection);
    recalcLayerout(borderList, sortParas.nWindDirection);
    recalcLayerout(hatchingList, sortParas.nWindDirection);

    // 根据扫描优先级合并列表
    if(0 == sortParas.nScanHatchingFirst) fileBlockList << supportList << borderList << hatchingList;
    else fileBlockList << supportList << hatchingList << borderList;

    return fileBlockList;
}


///
/// @brief 计算层位置因子
/// @param layerHei 层高
/// @param binFile 二进制文件
/// @param windDirection 风向(0-3)
/// @return 位置因子值
/// @details 实现步骤:
///   1. 获取指定层的信息
///   2. 根据风向计算位置因子:
///      - 0: Y坐标平方*10 + X坐标
///      - 1: X坐标平方*10 + Y坐标
///      - 2: -(Y坐标平方*10 + X坐标)
///      - 3: -(X坐标平方*10 + Y坐标)
///
double getLayerPosFactor(const int &layerHei, const BinFile &binFile, const int &windDirection)
{
    // 获取层信息
    auto layerPtr = binFile.layerMap[layerHei];

    // 根据风向计算位置因子
    switch(windDirection) {
    case 0:
    default:
        return pow(layerPtr.fCenterY * 10, 2) + layerPtr.fCenterX;
    case 1:
        return pow(layerPtr.fCenterX * 10, 2) + layerPtr.fCenterY;
    case 2:
        return - (pow(layerPtr.fCenterY * 10, 2) + layerPtr.fCenterX);
    case 3:
        return -(pow(layerPtr.fCenterX * 10, 2) + layerPtr.fCenterY);
    }
}


///
/// @brief 对单个文件的扫描块进行排序
/// @param sortParas 排序参数
/// @param layerHei 层高
/// @param listPartName 零件名称列表
/// @param binFileList 二进制文件列表
/// @return 排序后的文件块映射列表
/// @details 实现步骤:
///   1. 根据位置因子对文件进行预排序
///   2. 遍历排序后的文件列表
///   3. 对每个文件的数据块进行分类(支撑、边界、填充)
///   4. 对各分类进行风向排序
///   5. 按扫描优先级合并各文件的分类列表
///
QList<FileBlockMap> sortBlock_singleFile(const ScanSortTypes &sortParas, const int &layerHei,
                                       const QStringList &listPartName, const QList<BinFile> &binFileList)
{
    // 根据位置因子预排序文件
    QMultiMap<double, BinFile> tempBinFileList;
    for(const auto &binFile : binFileList)
    {
        if(binFile.layerMap.contains(layerHei))
        {
            tempBinFileList.insert(getLayerPosFactor(layerHei, binFile, sortParas.nWindDirection), binFile);
        }
    }

    // 处理排序后的文件
    QList<FileBlockMap> fileBlockList;
    for(const auto &binFile : tempBinFileList)
    {
        // 初始化分类列表
        QList<FileBlockMap> supportList;
        QList<FileBlockMap> borderList;
        QList<FileBlockMap> hatchingList;

        // 对数据块分类
        QString strFileID = Num2String::text(listPartName.indexOf(binFile.strFileName));
        for(const auto &block : binFile.layerMap[layerHei].blockList)
        {
            if(VecSupport & block->VectorTypeRef) supportList << FileBlockMap(strFileID, block);
            else if(VecBorder & block->VectorTypeRef) borderList << FileBlockMap(strFileID, block);
            else hatchingList << FileBlockMap(strFileID, block);
        }

        // 对各分类进行风向排序
        recalcLayerout(supportList, sortParas.nWindDirection);
        recalcLayerout(borderList, sortParas.nWindDirection);
        recalcLayerout(hatchingList, sortParas.nWindDirection);

        // 按扫描优先级合并列表
        if(0 == sortParas.nScanHatchingFirst) fileBlockList << supportList << borderList << hatchingList;
        else fileBlockList << supportList << hatchingList << borderList;
    }
    return fileBlockList;
}


///
/// @brief 根据排序策略选择扫描块排序方式
/// @param sortParas 排序参数
/// @param layerHei 层高
/// @param listPartName 零件名称列表
/// @param binFileList 二进制文件列表
/// @return 排序后的文件块映射列表
/// @details 实现步骤:
///   1. 根据nScanSortByPart参数选择排序方式:
///      - 0: 使用整体文件排序(sortBlock_totalFile)
///      - 其他: 使用单文件排序(sortBlock_singleFile)
///
QList<FileBlockMap> sortBlock(const ScanSortTypes &sortParas, const int &layerHei,
    const QStringList &listPartName, const QList<BinFile> &binFileList)
{
// 根据排序策略选择排序方式
if(0 == sortParas.nScanSortByPart) return sortBlock_totalFile(sortParas, layerHei, listPartName, binFileList);

return sortBlock_singleFile(sortParas, layerHei, listPartName, binFileList);
}

