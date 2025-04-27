#ifndef WRITEUFF_H
#define WRITEUFF_H

#include <QThread>
#include "publicheader.h"

class WriteUFF : public QThread
{
    Q_OBJECT

public:
    explicit WriteUFF(QObject *parent = nullptr);

public:
    void setUFFStatus(int);
    void setParaWriteBuff(PARAWRITEBUFF *);
    void stopThread() {
        m_bRunning = false;
    }

private:
    void run();

private:
    int m_nUFDataStatus;
    bool m_bRunning = true;
    PARAWRITEBUFF *_writeBuff = nullptr;
};

#endif // WRITEUFF_H
