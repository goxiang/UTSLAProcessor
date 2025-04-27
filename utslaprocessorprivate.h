#ifndef UTSLAPROCESSORPRIVATE_H
#define UTSLAPROCESSORPRIVATE_H

#include "utslaprocessor.h"
#include <QFutureWatcher>
#include <QSharedPointer>
#include <QJsonObject>
#include <QSemaphore>
#include <QJsonArray>

#include "algorithmapplication.h"

struct LatticeLayerInfo;
struct WriterBufferParas;
class SelfAdaptiveModule;
class LatticeInterface;
class QJsonParsing;
class FileWriter;

typedef QSharedPointer<FileWriter> USPFileWriterPtr;
typedef QSharedPointer<AlgorithmApplication> AlgrithmPtr;

class UTSLAProcessorPrivate
{
public:
    UTSLAProcessorPrivate(UTSLAProcessor *);

public:
    void processing(const int &, const int &, const QJsonObject &);
    void stopBuildProcessing();
    bool loadParameters(const QString &, const QString &);

private:
    void building(const int &, const int &);
    void buildProcessing();
    void partProcessing(const int &);
    void readDatas(const int &);
    void calcLayerDatas(const int &, const int &, const int &, const USPFileWriterPtr &, SOLIDPATH &);
    void readLayerDatas(const int &, const int &, const USPFileWriterPtr &, SOLIDPATH &);

    int getPreLayerHei(const int &, const int &);
    int getNextLayerHei(const int &, const int &);

    void updateLayerSplicingInfo();
    void parsingDatas(const int &, QList<AREAINFOPTR> &);
    void updateProgress();
    void updateProgress(const double &);

    void createJsonResult(QJsonObject &);
    void createJsonResultWithCompress(QJsonObject &);
    void createJobFileResult(QJsonObject &);

    void loadExportParameters();
    void calcSLMScale(QList<AREAINFOPTR> &);

    bool createFileWriter(const int &, USPFileWriterPtr &);
    bool createFileWriter(const int &, const QString &, USPFileWriterPtr &);

    void initializeLattice();
    void createLattice(Paths &, const Paths &, const int &, Paths &, LatticeLayerInfo &);

private:
    friend class DividerProcessor;
    Q_DECLARE_PUBLIC(UTSLAProcessor)
    UTSLAProcessor * const q_ptr = nullptr;

    QString strBPPName = "";
    QFutureWatcher<void> futureWatcher;

    QSharedPointer<SelfAdaptiveModule> _selfAdaptiveModule = nullptr;
    QSharedPointer<WriterBufferParas> _writerBufferParas = nullptr;
    QSharedPointer<LatticeInterface> _latticeInfPtr = nullptr;

    int _progressPos = -1;
    double fProcessing = 0.0;
    double fSinglePartRatio = 1.0;
    QJsonArray objZipDescFile;

    int nExportFileMode = EXPORT_NORMAL;
    double fScaleX = 1.0;
    double fScaleY = 1.0;
    int _threadCount = 1;
};

#endif // UTSLAPROCESSORPRIVATE_H
