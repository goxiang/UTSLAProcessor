#ifndef USPFILEWRITER_H
#define USPFILEWRITER_H

class QFile;
class WriteUFF;
class AlgorithmApplication;
struct BOUNDINGBOX;
struct BOUNDINGRECT;
struct PARAWRITEBUFF;

#include <QSharedPointer>
#include <QString>
class QJsonObject;

class FileWriter {
public:
    FileWriter() = default;
    ~FileWriter() = default;

    virtual bool initFileWrite() { return true; }

    virtual void startBuffWriter() {}
    virtual void waitBuffWriter() {}

    virtual void writeLayerInfo(const int &, const BOUNDINGRECT &, const double &) {}
    virtual void writeAreaInfo(const int &) {}
    virtual void writeFileEnd() {}
    virtual void writeFileBegin_SomePropertys() {}
    virtual void createUSPFile(const QString &, const QString &, const float &, BOUNDINGBOX *,
                       const int &, const int &, QJsonObject &) {}
    virtual inline PARAWRITEBUFF *getBufPara() { return nullptr; }
    virtual AlgorithmApplication *algo() { return nullptr; }
    virtual void addVolume(const double &) {}
    virtual void setScannerIndex(const int &) {}

    virtual QSharedPointer<FileWriter> cloneWriter() { return nullptr; }

    static QByteArray getFileHash(const QString &);
};

///
/// ! @coreclass{USPFileWriter}
/// 文件写入器实现类,继承自FileWriter类
///
class USPFileWriter : public FileWriter
{
public:
    USPFileWriter();
    ~USPFileWriter();

    bool initFileWrite();

    void startBuffWriter();
    void waitBuffWriter();

    void writeLayerInfo(const int &, const BOUNDINGRECT &, const double &);
    void writeAreaInfo(const int &);
    void writeFileEnd();
    void writeFileBegin_SomePropertys();
    void createUSPFile(const QString &, const QString &, const float &, BOUNDINGBOX *,
                       const int &, const int &, QJsonObject &);

    inline PARAWRITEBUFF *getBufPara() { return _writeBuff; }
    AlgorithmApplication *algo() { return algorithm; }
    void addVolume(const double &volume) { m_fTotalVolume += volume; }
    QSharedPointer<FileWriter> cloneWriter();

private:
    void writeProperty(const int &);
    void writeProperty(const int &, const int &);
    void writeFlush();
    void writeXMLInfo(QFile *, const QByteArray &, const QString &, const QString &,
                      const float &, BOUNDINGBOX *, const int &, const int &);
    void writeJsonInfo(const QString &, const QString &,
                       const float &, BOUNDINGBOX *, const int &, const int &, QJsonObject &);

public:
    WriteUFF *fileWriter = nullptr;
    PARAWRITEBUFF *_writeBuff = nullptr;
    AlgorithmApplication *algorithm = nullptr;

    QString m_strSystemTime;
    double m_fTotalVolume = 0.0;
};

#endif // USPFILEWRITER_H
