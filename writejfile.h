#ifndef WRITEJFILE_H
#define WRITEJFILE_H

#include <QThread>
#include "publicheader.h"

namespace JFileDef {
struct JFileLayerInfo;
}

class WriteJFile : public QThread
{
    Q_OBJECT

public:
    explicit WriteJFile(QObject *parent = nullptr);

public:
    void setJFileLayerInfo(JFileDef::JFileLayerInfo *);
    void setUFFStatus(int);
    void setParaWriteBuff(PARAWRITEBUFF *);
    void stopThread() {
        m_bRunning = false;
    }

private:
    void run();

private:
    friend class SLJobFileWriter;
    int m_nUFDataStatus;
    int m_nScannerIndex = 0;
    bool m_bRunning = true;
    PARAWRITEBUFF *_writeBuff = nullptr;

    JFileDef::JFileLayerInfo *jFLayerInfo = nullptr;
};

#endif // WRITEJFILE_H
