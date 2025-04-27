#ifndef SLAPROCESSOREXTEND_H
#define SLAPROCESSOREXTEND_H

#include <QObject>
#include "UTSLAProcessor_global.h"

///
/// ! @coreclass{SLAProcessorExtend}
/// 提供绘制和显示功能,继承自QObject
///
class SLAProcessorExtend : public QObject
{
public:
    SLAProcessorExtend();
    ~SLAProcessorExtend();
#ifdef USE_PROCESSOR_EXTEND
    void setCallbackFun(funDrawLine &&, funDrawColorLine &&);
    void setCallbackFun(funClearCanvas &&);

    void drawLines(const qint64 &, const qint64 &, const qint64 &, const qint64 &, const int &);
    void drawLines(const int &, const int &, const int &, const int &, const int &);

    void drawLines(const qint64 &, const qint64 &, const qint64 &, const qint64 &, const quint8 &r,
                   const quint8 &g, const quint8 &b);
    void drawLines(const int &x1, const int &y1, const int &x2,
                   const int &y2, const quint8 &r, const quint8 &g, const quint8 &b);

    void setPenColor(const quint8 &r, const quint8 &g, const quint8 &b);
    void drawMark(const qint64 &, const qint64 &);
    void drawMark(const int &x1, const int &y1);


    void clearCanvas(const int &);

public:
    funDrawLine funDraw = nullptr;
    funDrawColorLine funDrawColor = nullptr;
    funClearCanvas funClear = nullptr;

    int nLastX = 0.0;
    int nLastY = 0.0;
    quint8 nR = 0;
    quint8 nG = 0;
    quint8 nB = 0;
#endif
};

#endif // SLAPROCESSOREXTEND_H
