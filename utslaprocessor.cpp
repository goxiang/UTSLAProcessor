#include "utslaprocessor.h"
#include "utslaprocessorprivate.h"
#include "utbpparareader.h"
#include "meshinfo.h"

#include <QTimer>
#include <QJsonObject>
#include <QThread>
#include <QDateTime>

///
/// @brief 创建SLA处理器工厂函数
/// @return 新创建的UTSLAProcessor对象指针
/// @details 实现步骤:
///   1. 创建并返回UTSLAProcessor实例
///   2. 返回的指针需要调用方负责释放
///
UTBPProcessBase *createSLAProcessor()
{
    // 创建并返回新的SLA处理器实例
    return new UTSLAProcessor();
}

#ifdef USE_PROCESSOR_EXTEND
SLAProcessorExtend *glProcessor = nullptr;
#endif
UTSLAProcessor::UTSLAProcessor()
    : d_ptr(new UTSLAProcessorPrivate(this))
{
    glMeshInfo;
#ifdef USE_PROCESSOR_EXTEND
    glProcessor = this;
#endif
}

UTSLAProcessor::~UTSLAProcessor()
{
    stopProcessing();
    d_ptr->futureWatcher.waitForFinished();
}
//#define USE_BPP_SCRIPT_TEST_LOG
void UTSLAProcessor::processing(int nSLayer, int nELayer, const QJsonObject &jsonObj)
{
#ifdef USE_BPP_SCRIPT_TEST_LOG
    QString strMsg;
    QDebug debug(&strMsg);
    debug << d_ptr->sUFFBuildParas;
    QFile file("D:/BPPScriptTestLog/Log/bppackage.log");
    if(file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        file.write((QDateTime::currentDateTime().toString("yyyy-MM-dd HH::mm:ss.zzz")
                    + "\t" + strMsg).toLocal8Bit());
        file.close();
    }
#else
    d_ptr->processing(nSLayer, nELayer, jsonObj);
#endif
}

void UTSLAProcessor::stopBuildProcessing()
{
    d_ptr->stopBuildProcessing();
}

bool UTSLAProcessor::loadParameters(const QString &strBppPath, const QString &strIniPath)
{
    return d_ptr->loadParameters(strBppPath, strIniPath);
}

bool UTSLAProcessor::replacePrameters(const QJsonObject &obj)
{
    if(obj.isEmpty()) return false;

    UTBPParaReader::replaceParameters(obj, *(d_ptr->_writerBufferParas->_bppParaPtr.data()));
    return true;
}

#ifdef USE_PROCESSOR_EXTEND
void UTSLAProcessor::setDisplayFun(funDrawLine f1, funDrawColorLine f2, funClearCanvas f3)
{
    setCallbackFun(std::move(f1), std::move(f2));
    setCallbackFun(std::move(f3));
}

void UTSLAProcessor::testProcessing(const QJsonObject &obj)
{
    QTimer::singleShot(0, this, [=](){
        this->startProcessing(obj);
    });
}
#endif
