#ifndef SLJOBFILEWRITER_H
#define SLJOBFILEWRITER_H

#include "uspfilewriter.h"
#include "qjsonparsing.h"
#include "writejfile.h"

namespace JFileDef {
struct JFileLayerInfo;
}
class QXmlStreamWriter;

struct BlockInfo;
typedef QSharedPointer<BlockInfo> BlockInfoPtr;

struct Layer {
    int Z = 0;
    float _totalArea = 0.0f;
    float fMinX = FLT_MAX;
    float fMinY = FLT_MAX;
    float fMaxX = FLT_MIN;
    float fMaxY = FLT_MIN;
    float fCenterX = 0.0;
    float fCenterY = 0.0;
    QList<BlockInfoPtr> blockList;
};

struct BinFile {
    QString strFileName;
    QMap<int, Layer> layerMap;
};

///
/// ! @coreclass{SLJobFileWriter}
/// 负责将打印数据写入特定格式的作业文件,继承自FileWriter类
///
class SLJobFileWriter : public FileWriter
{
public:
    SLJobFileWriter();
    ~SLJobFileWriter();

    bool initFileWrite();

    void startBuffWriter();
    void waitBuffWriter();

    void writeLayerInfo(const int &, const BOUNDINGRECT &, const double &);
    void createUSPFile(const QString &, const QString &, const float &, BOUNDINGBOX *,
                       const int &, const int &, QJsonObject &);

    inline PARAWRITEBUFF *getBufPara() { return buffPara; }
    AlgorithmApplication *algo() { return algorithm; }
    void setScannerIndex(const int &);

    static void createJFileContentXml(const QString &, const QStringList &, const QString &,
                                      const QSharedPointer<WriterBufferParas> &);
    static void createJFileMetaDataXML(const QString &, const QStringList &, const QString &,
                                       const QSharedPointer<WriterBufferParas> &);


    static void addBinFileToContentXML(QXmlStreamWriter *, const QString &, const QString &);
    static void addParemeterToMetaDataXML(QXmlStreamWriter *, const int &, const int &, const float &, const float &, const float &);
    static void addBinFileToMetaDataXML(QXmlStreamWriter *, const QString &, const int &, const QString &, const qint64 &);
    static void addPartToMetaDataXML(QXmlStreamWriter *, const int &, const QString &, const float &, const float &,
                                     const float &, const float &, const float &, const float &);
    static void addBlockToMetaDataXML(QXmlStreamWriter *, const QString &, const BlockInfoPtr &);

    static void loadJsonFiles(const QString &path, const QStringList &, QList<BinFile> &, int &, int &, double &);
    static int getVectorType(const int &);

public:
    WriteJFile *fileWriter = nullptr;
    PARAWRITEBUFF *buffPara = nullptr;
    AlgorithmApplication *algorithm = nullptr;

    JFileDef::JFileLayerInfo *curLayerInfo = nullptr;
    QMap<int, QSharedPointer<JFileDef::JFileLayerInfo>> layerInfoPtrMap;
};

#endif // SLJOBFILEWRITER_H
