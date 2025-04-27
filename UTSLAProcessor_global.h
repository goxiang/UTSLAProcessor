#ifndef UTSLAPROCESSOR_GLOBAL_H
#define UTSLAPROCESSOR_GLOBAL_H

#include <QtCore/qglobal.h>

//#define USE_PROCESSOR_EXTEND
#ifdef USE_PROCESSOR_EXTEND
#include <functional>
typedef std::function<void(int, int, int, int, int)> funDrawLine;
typedef std::function<void(int, int, int, int, quint8, quint8, quint8)> funDrawColorLine;
typedef std::function<void(int)> funClearCanvas;

class SLAProcessorExtend;
extern SLAProcessorExtend *glProcessor;
#endif

#endif // UTSLAPROCESSOR_GLOBAL_H
