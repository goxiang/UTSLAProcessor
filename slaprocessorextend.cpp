#include "slaprocessorextend.h"

#define DUNIT 0.01
#include <QThread>
#include <QDebug>

SLAProcessorExtend::SLAProcessorExtend()
{
    this->moveToThread(new QThread());
    this->thread()->start();
}

SLAProcessorExtend::~SLAProcessorExtend()
{
    this->thread()->quit();
    this->thread()->wait();
}
#ifdef USE_PROCESSOR_EXTEND
void SLAProcessorExtend::setCallbackFun(funDrawLine &&f, funDrawColorLine &&f2)
{
    funDraw = std::move(f);
    funDrawColor = std::move(f2);
}

void SLAProcessorExtend::setCallbackFun(funClearCanvas &&f)
{
    funClear = std::move(f);
}

void SLAProcessorExtend::drawLines(const qint64 &x1, const qint64 &y1, const qint64 &x2,
                                   const qint64 &y2, const int &type)
{
    drawLines(int(x1), int(y1), int(x2), int(y2), type);
}

void SLAProcessorExtend::drawLines(const int &x1, const int &y1, const int &x2,
                                   const int &y2, const int &type)
{
    if(funDraw) funDraw(qRound(x1 * DUNIT), qRound(y1 * DUNIT), qRound(x2 * DUNIT),
                        qRound(y2 * DUNIT), type);
}

void SLAProcessorExtend::drawLines(const qint64 &x1, const qint64 &y1, const qint64 &x2,
                                   const qint64 &y2, const quint8 &r, const quint8 &g, const quint8 &b)
{
    drawLines(int(x1), int(y1), int(x2), int(y2), r, g, b);
}

void SLAProcessorExtend::drawLines(const int &x1, const int &y1, const int &x2,
                                   const int &y2, const quint8 &r, const quint8 &g, const quint8 &b)
{
    if(funDrawColor) funDrawColor(qRound(x1 * DUNIT), qRound(y1 * DUNIT), qRound(x2 * DUNIT),
                        qRound(y2 * DUNIT), r, g, b);
}

void SLAProcessorExtend::setPenColor(const quint8 &r, const quint8 &g, const quint8 &b)
{
    nR = r;
    nG = g;
    nB = b;
}

void SLAProcessorExtend::drawMark(const qint64 &x, const qint64 &y)
{
    drawMark(int(x), int(y));
}

void SLAProcessorExtend::drawMark(const int &x, const int &y)
{
    if(funDrawColor) funDrawColor(qRound(nLastX * DUNIT), qRound(nLastY * DUNIT),
                                  qRound(x * DUNIT), qRound(y * DUNIT), nR, nG, nB);
    nLastX = x;
    nLastY = y;
}

void SLAProcessorExtend::clearCanvas(const int &nHei)
{
    if(funClear) funClear(nHei);
}
#endif
