#ifndef UTSLAPROCESSOR_H
#define UTSLAPROCESSOR_H

#include <QObject>

#if defined(UTSLAPROCESSOR_LIBRARY)
#  define UTSLAPROCESSOR_EXPORT Q_DECL_EXPORT
#else
#  define UTSLAPROCESSOR_EXPORT Q_DECL_IMPORT
#endif

#include "UTSLAProcessor_global.h"
#include "utbpprocessbase.h"

#ifdef USE_PROCESSOR_EXTEND
#include "slaprocessorextend.h"
#endif
class UTSLAProcessorPrivate;

///
/// ! @coreclass{UTSLAProcessor}
/// 核心处理类，继承自UTBPProcessBase
///
class UTSLAPROCESSOR_EXPORT UTSLAProcessor : public UTBPProcessBase
#ifdef USE_PROCESSOR_EXTEND
        ,public SLAProcessorExtend
#endif
{
public:
    explicit UTSLAProcessor();
    ~UTSLAProcessor();

#ifdef USE_PROCESSOR_EXTEND
    void setDisplayFun(funDrawLine, funDrawColorLine, funClearCanvas);
    void testProcessing(const QJsonObject &);
#endif

protected:
    void processing(int, int, const QJsonObject &);
    void stopBuildProcessing();
    bool loadParameters(const QString &, const QString &);
    bool replacePrameters(const QJsonObject &);

protected:
    Q_DECLARE_PRIVATE(UTSLAProcessor)
    QSharedPointer<UTSLAProcessorPrivate> d_ptr = nullptr;

    friend class DividerProcessor;
};
#endif // UTSLAPROCESSOR_H
