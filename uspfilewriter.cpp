#include "uspfilewriter.h"
#include "writeuff.h"
#include "publicheader.h"
#include "algorithmapplication.h"

#include <QFileInfo>
#include <QDateTime>
#include <QJsonObject>
#include <QXmlStreamWriter>
#include <QCryptographicHash>

///
/// @brief USP文件写入器构造函数
/// @details 实现步骤:
///   1. 创建文件写入线程
///   2. 创建写入缓冲区
///   3. 创建算法应用对象
///
USPFileWriter::USPFileWriter()
{
    // 创建UFF写入线程
    fileWriter = new WriteUFF();
    
    // 创建写入缓冲区
    _writeBuff = new PARAWRITEBUFF();
    
    // 创建算法应用对象
    algorithm = new AlgorithmApplication();
}

///
/// @brief USP文件写入器析构函数
/// @details 实现步骤:
///   1. 等待写入线程完成
///   2. 释放写入器资源
///   3. 释放缓冲区
///   4. 释放算法对象
///
USPFileWriter::~USPFileWriter()
{
    // 等待并释放文件写入线程
    if(fileWriter)
    {
        fileWriter->wait();
        delete fileWriter;
        fileWriter = nullptr;
    }

    // 释放写入缓冲区
    if(_writeBuff)
    {
        delete _writeBuff;
        _writeBuff = nullptr;
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
/// @return 初始化是否成功
/// @details 实现步骤:
///   1. 设置算法和写入器的缓冲区
///   2. 记录系统时间
///   3. 创建文件数据结构
///
bool USPFileWriter::initFileWrite()
{
    // 设置算法缓冲区
    algorithm->setBuffParas(_writeBuff);
    // 设置写入器缓冲区
    fileWriter->setParaWriteBuff(_writeBuff);
    // 记录当前时间
    m_strSystemTime = QDateTime::currentDateTime().toString("yyMMdd hh:mm:ss");
    // 创建文件数据
    return _writeBuff->createUFileData();
}

///
/// @brief 启动缓冲区写入
/// @details 设置写入状态并启动写入线程
///
void USPFileWriter::startBuffWriter()
{
    // 设置开始状态
    fileWriter->setUFFStatus(UFFWRITE_BEGIN);
    // 启动写入线程
    fileWriter->start();
}

///
/// @brief 等待写入完成
/// @details 实现步骤:
///   1. 设置结束状态
///   2. 等待写入完成
///   3. 写入层结束标记
///
void USPFileWriter::waitBuffWriter()
{
    // 设置结束状态
    fileWriter->setUFFStatus(UFFWRITE_END);
    // 等待写入完成
    fileWriter->wait();
    // 写入层结束标记
    writeProperty(SECTION_LAYEREND);
}


///
/// @brief 写入层信息
/// @param nHei 层高度
/// @param totalRc 边界矩形
/// @param unused 未使用参数
/// @details 实现步骤:
///   1. 写入层号
///   2. 写入X最小值
///   3. 写入X最大值
///   4. 写入Y最小值
///   5. 写入Y最大值
///
void USPFileWriter::writeLayerInfo(const int &nHei, const BOUNDINGRECT &totalRc, const double &)
{
    // 写入层号
    writeProperty(SECTION_LAYER, nHei);
    // 写入X边界
    writeProperty(SECTION_MINX, totalRc.minX);
    writeProperty(SECTION_MAXX, totalRc.maxX);
    // 写入Y边界
    writeProperty(SECTION_MINY, totalRc.minY);
    writeProperty(SECTION_MAXY, totalRc.maxY);
}

///
/// @brief 写入区域信息
/// @param area 区域值
/// @details 写入层区域属性
///
void USPFileWriter::writeAreaInfo(const int &area)
{
    writeProperty(SECTION_LAYERAREAS, area);
}

///
/// @brief 写入文件结束标记
/// @details 实现步骤:
///   1. 写入结束段标识
///   2. 写入结束字符串
///   3. 刷新缓冲区
///
void USPFileWriter::writeFileEnd()
{
    writeProperty(SECTION_FILEEND);
    _writeBuff->gFile.write(QString("FileEnding").toLocal8Bit());
    writeFlush();
}

///
/// @brief 写入文件起始属性
/// @details 写入构建速度显示标志
///
void USPFileWriter::writeFileBegin_SomePropertys()
{
    writeProperty(BppParas->sGeneralPara.nShowBuildSpeed ? 1 : 0);
}


///
/// @brief 创建USP文件
/// @param fileName 文件名
/// @param bppName BPP文件名
/// @param fThickness 层厚
/// @param boundingBox 边界框指针
/// @param minHei 最小高度
/// @param maxHei 最大高度
/// @param jsonObj JSON配置对象
/// @details 实现步骤:
///   1. 关闭并清理临时文件
///   2. 创建目标USP文件
///   3. 写入XML和JSON信息
///   4. 分块复制临时文件数据
///   5. 完成文件创建并删除临时文件
///
void USPFileWriter::createUSPFile(const QString &fileName, const QString &bppName,
                                 const float &fThickness, BOUNDINGBOX *boundingBox,
                                 const int &minHei, const int &maxHei,
                                 QJsonObject &jsonObj)
{
    // 关闭已打开的临时文件
    if(_writeBuff->gFile.isOpen())
    {
        _writeBuff->gFile.close();
    }

    // 准备USP文件路径
    QString strTempFile = _writeBuff->gFile.fileName();
    QString strUspFile = strTempFile.left(strTempFile.size() - 3) + "usp";
    QFile file(strUspFile);
    if(file.exists()) file.remove();

    // 创建并写入USP文件
    if(file.open(QIODevice::ReadWrite))
    {
        // 写入XML和JSON信息
        writeXMLInfo(&file, getFileHash(_writeBuff->gFile.fileName()),
                    fileName, bppName, fThickness, boundingBox, minHei, maxHei);
        writeJsonInfo(fileName, bppName, fThickness, boundingBox, minHei, maxHei, jsonObj);

        // 分块复制临时文件数据
        if(_writeBuff->gFile.open(QIODevice::ReadOnly))
        {
            qint64 nFileSz = _writeBuff->gFile.size();
            char dataBuf[READMAXSIZE] = {0};
            qint64 nReadSz = 0;

            while(nFileSz > 0)
            {
                // 计算当前读取块大小
                nReadSz = READMAXSIZE < nFileSz ? READMAXSIZE : nFileSz;
                // 读取并写入数据块
                _writeBuff->gFile.read(dataBuf, nReadSz);
                file.write(dataBuf, nReadSz);
                nFileSz -= nReadSz;
            }
            _writeBuff->gFile.close();
        }

        // 完成文件写入
        file.flush();
        file.close();
        QThread::msleep(5);
    }

    // 删除临时文件
    _writeBuff->gFile.remove();
}


//private
///
/// @brief 写入属性键
/// @param key 属性键值
/// @details 写入8位属性标识
///
void USPFileWriter::writeProperty(const int &key)
{
    writeData8(&_writeBuff->gFile, qint8(key));
}

///
/// @brief 写入属性键值对
/// @param key 属性键值
/// @param value 属性数值
/// @details 实现步骤:
///   1. 写入8位属性标识
///   2. 写入32位属性值
///
void USPFileWriter::writeProperty(const int &key, const int &value)
{
    writeData8(&_writeBuff->gFile, qint8(key));
    writeData32(&_writeBuff->gFile, value);
}

void USPFileWriter::writeFlush()
{
    _writeBuff->gFile.close();
}

///
/// @brief 写入XML格式的文件信息
/// @param mFile 目标文件指针
/// @param fileHash 文件哈希值
/// @param fileName 文件名
/// @param bppName BPP文件名
/// @param fThickness 层厚
/// @param bBox 边界框
/// @param minHei 最小高度
/// @param maxHei 最大高度
/// @details 实现步骤:
///   1. 初始化XML写入器
///   2. 写入文件基本信息
///   3. 写入边界信息
///   4. 写入功率配置
///   5. 写入拼接信息
///   6. 写入多层速度因子
///
void USPFileWriter::writeXMLInfo(QFile *mFile, const QByteArray &fileHash,
                                const QString &fileName, const QString &bppName,
                                const float &fThickness, BOUNDINGBOX *bBox,
                                const int &minHei, const int &maxHei)
{
    // 初始化XML写入器
    QXmlStreamWriter writer(mFile);
    writer.setCodec("gb2313");
    writer.setAutoFormatting(true);
    writer.writeStartDocument();

    // 写入文件头信息
    writer.writeStartElement("FileInfo");
    writer.writeAttribute("Type", "Union ScanPathFile");
    writer.writeAttribute("Version", "2.8");

    // 写入系统时间
    writer.writeStartElement("SystemTime");
    writer.writeCharacters(m_strSystemTime);
    writer.writeEndElement();

    // 写入文件哈希值
    writer.writeStartElement("MD5");
    writer.writeCharacters(fileHash);
    writer.writeEndElement();

    // 写入BPP信息
    writer.writeStartElement("BPPName");
    writer.writeCharacters(bppName.toUtf8());
    writer.writeEndElement();

    // 写入层厚信息
    writer.writeStartElement("LayerThinkness");
    writer.writeCharacters(QString("%1").arg(double(fThickness)));
    writer.writeEndElement();

    // 写入边界框信息
    if(bBox)
    {
        // 写入X范围
        writer.writeStartElement("MinX");
        writer.writeCharacters(QString("%1").arg(bBox->minX));
        writer.writeEndElement();
        writer.writeStartElement("MaxX");
        writer.writeCharacters(QString("%1").arg(bBox->maxX));
        writer.writeEndElement();

        // 写入Y范围
        writer.writeStartElement("MinY");
        writer.writeCharacters(QString("%1").arg(bBox->minY));
        writer.writeEndElement();
        writer.writeStartElement("MaxY");
        writer.writeCharacters(QString("%1").arg(bBox->maxY));
        writer.writeEndElement();

        // 写入高度范围
        writer.writeStartElement("MinHei");
        writer.writeCharacters(QString("%1").arg(minHei));
        writer.writeEndElement();
        writer.writeStartElement("MaxHei");
        writer.writeCharacters(QString("%1").arg(maxHei));
        writer.writeEndElement();
    }

    // 写入总体信息
    writer.writeStartElement("TotalHei");
    writer.writeCharacters(QString("%1").arg(maxHei));
    writer.writeEndElement();

    writer.writeStartElement("TotalVolume");
    writer.writeCharacters(QString::number(m_fTotalVolume, 'f', 2));
    writer.writeEndElement();

    writer.writeStartElement("Unit");
    writer.writeCharacters(QString("%1").arg(LAYERHEIFACTOR));
    writer.writeEndElement();

    // 写入零件信息
    writer.writeStartElement("FileCnt");
    writer.writeCharacters(QString("%1").arg(1));
    writer.writeEndElement();

    for(int iIndex = 0; iIndex < 1; iIndex ++)
    {
        writer.writeEmptyElement("Part");
        writer.writeAttribute("ID", QString::number(iIndex));
        writer.writeAttribute("Name", fileName);
        writer.writeAttribute("MaxHei", QString::number(maxHei * 0.1));
        writer.writeAttribute("Thickness", QString::number(double(fThickness)));
    }

    // 写入功率配置
    writer.writeStartElement("PowerCnt");
    writer.writeCharacters(QString("%1").arg(BppParas->sGeneralPara.nNumber_Power));
    writer.writeEndElement();

    for(int iIndex = 0; iIndex < BppParas->sGeneralPara.nNumber_Power; iIndex ++)
    {
        writer.writeEmptyElement("nPower");
        writer.writeAttribute("ID", QString::number(iIndex));
        writer.writeAttribute("Value", QString::number(BppParas->sGeneralPara.nLaserPower[iIndex]));
    }

    // 写入拼接类型
    writer.writeStartElement("SplicingType");
    int nSplicingType = 0;
    if(BppParas->sGeneralPara.nUseSplicingMode ||
        _writeBuff->getExtendedValue<int>("Splicing/nScanRangeMode", 0))
    {
        nSplicingType = (1 << BpcParas->nNumber_SplicingScanner) - 1;
    }
    writer.writeCharacters(QString("%1").arg(nSplicingType));
    writer.writeEndElement();

    // 写入多层速度因子
    writer.writeStartElement("MultiLayerSpeedFactor");
    for(int iIndex = 0; iIndex < 5; iIndex ++)
    {
        writer.writeEmptyElement("fMLSpeedFactor");
        writer.writeAttribute("ID", QString::number(iIndex));
        writer.writeAttribute("LaserPower", QString::number(BppParas->sGeneralPara.nLaserPower[
                                                            BppParas->sGeneralPara.nMultiLayerPowerIndex[iIndex]]));
        writer.writeAttribute("SpeedFactor", QString::number(BppParas->sGeneralPara.fMultiLayerSpeedFactor[iIndex]));
    }
    writer.writeEndElement();

    // 结束文档
    writer.writeEndElement();
    writer.writeEndDocument();
}


///
/// @brief 写入JSON格式的文件信息
/// @param fileName 文件名
/// @param bppName BPP文件名
/// @param fThickness 层厚
/// @param bBox 边界框
/// @param minHei 最小高度
/// @param maxHei 最大高度
/// @param jsonInfo JSON对象引用
/// @details 实现步骤:
///   1. 写入基本文件信息
///   2. 写入单位和层厚
///   3. 写入边界框信息
///   4. 写入总体参数
///   5. 写入拼接配置
///
void USPFileWriter::writeJsonInfo(const QString &fileName, const QString &bppName,
                                 const float &fThickness, BOUNDINGBOX *bBox,
                                 const int &minHei, const int &maxHei, 
                                 QJsonObject &jsonInfo)
{
    // 写入基本文件信息
    jsonInfo["BPPName"] = bppName;
    jsonInfo["PartName"] = fileName;
    
    // 写入单位和层厚
    jsonInfo["Unit"] = LAYERHEIFACTOR;
    jsonInfo["LayerThinkness"] = double(fThickness);

    // 写入边界框信息
    if(bBox)
    {
        jsonInfo["MinX"] = bBox->minX;
        jsonInfo["MaxX"] = bBox->maxX;
        jsonInfo["MinY"] = bBox->minY;
        jsonInfo["MaxY"] = bBox->maxY;
        jsonInfo["MinHei"] = minHei;
        jsonInfo["MaxHei"] = maxHei;
    }

    // 写入总体参数
    jsonInfo["TotalHei"] = maxHei;
    jsonInfo["TotalVolume"] = m_fTotalVolume;

    // 计算并写入拼接类型
    int nSplicingType = 0;
    if(BppParas->sGeneralPara.nUseSplicingMode ||
        _writeBuff->getExtendedValue<int>("Splicing/nScanRangeMode", 0))
    {
        nSplicingType = (1 << BpcParas->nNumber_SplicingScanner) - 1;
    }
    jsonInfo["SplicingType"] = nSplicingType;
}


///
/// @brief 计算文件的MD5哈希值
/// @param strPath 文件路径
/// @return 返回十六进制格式的哈希值
/// @details 实现步骤:
///   1. 初始化MD5哈希对象
///   2. 读取文件数据
///   3. 计算哈希值
///   4. 转换为十六进制
///
QByteArray FileWriter::getFileHash(const QString &strPath)
{
    // 初始化结果数组
    QByteArray byteArray = "";
    // 创建MD5哈希对象
    QCryptographicHash dataMark(QCryptographicHash::Md5);

    // 打开并读取文件
    QFile file(strPath);
    if(file.open(QIODevice::ReadOnly))
    {
        // 计算文件数据的哈希值
        dataMark.addData(&file);
        // 转换为十六进制格式
        byteArray = dataMark.result().toHex();
        file.close();
    }
    return byteArray;
}

///
/// @brief 克隆文件写入器
/// @return 返回新的文件写入器智能指针
/// @details 实现步骤:
///   1. 创建新的USP写入器
///   2. 使用智能指针管理
///   3. 返回克隆的写入器
///
QSharedPointer<FileWriter> USPFileWriter::cloneWriter()
{
    // 创建新的USP写入器并用智能指针管理
    auto fileWriter = QSharedPointer<FileWriter>(new USPFileWriter);
    return fileWriter;
}

