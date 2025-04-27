#ifndef PUBLICHEADER_H
#define PUBLICHEADER_H

//#define MaxScanLayers 1

enum EXPORT_FILE_MODE {
    EXPORT_NORMAL = 0,
    EXPORT_SLM_BUILDFILE
};

#define DEF_PI      3.14159265358979323846

#include "uspfiledef.h"
#include "bppbuildparameters.h"
#include "slmsplicingmodule.h"
#include "slascaninfodef.h"
#include "pathparameters.h"
#include "qjsonparsing.h"

#include <QJsonObject>
#include <QMutex>
#include <QFile>
#include <QRect>

#define FILEDATAUNIT UNITSPRECISION
#define COMMONUNIT FILEDATAUNIT

/// 
/// @brief 文件数据单位转换宏定义
/// @details 用于在不同尺度间转换的系数
///

// 层高度转换因子(1/FILEDATAUNIT)
#define LAYERHEIFACTOR      (1.0 / FILEDATAUNIT)

// 面积转换因子(1/FILEDATAUNIT^2) 
#define AREAFACTOR          (1.0 / FILEDATAUNIT / FILEDATAUNIT)

// 文件数据单位的0.2倍
#define FILEDATAUNIT_0_2    (FILEDATAUNIT * 0.2)

// 文件数据单位的0.4倍
#define FILEDATAUNIT_0_4    (FILEDATAUNIT * 0.4)

// 文件数据单位的0.5倍
#define FILEDATAUNIT_0_5    (FILEDATAUNIT * 0.5)

// 文件数据单位的0.05倍
#define FILEDATAUNIT_0_05   (FILEDATAUNIT * 0.05)

// 最小过滤面积(0.1 * FILEDATAUNIT)^2
#define MINFILTERAREA       ((FILEDATAUNIT * 0.1) * (FILEDATAUNIT * 0.1))

#define SPLICING_ERROR_HOR      3
#define SPLICING_ERROR_HORx2    ((SPLICING_ERROR_HOR - 1) << 1)

#define BppParas _writeBuff->_writerBufferParas->_bppParaPtr
#define BpcParas _writeBuff->_writerBufferParas->_bpcParaPtr
#define SplicingPtr _writeBuff->_writerBufferParas->_splicingPtr
#define ExtendedObj _writeBuff->_writerBufferParas->_extendObj
#define ExtendedParas _writeBuff->getExtendedValue

extern void writeData8(QFile *, qint8);
extern void writeData32(QFile *, qint32);
extern void writeScanLines(QFile *, QVector<SCANLINE> &);

typedef QSharedPointer<UFILEDATA> UFILEDATAPTR;

///
/// @brief 填充光束类型枚举
/// @details 定义不同填充策略的光束类型
///
enum HATCHINGBEAM {
    HATCHINGBEAM_NORMAL = 0,    // 普通填充光束
    HATCHINGBEAM_SMALL = 1,     // 小区域填充光束
    HATCHINGBEAM_REMAINING = 2  // 剩余区域填充光束
};


struct SplicingInfo {
    int64_t _splicingPos;
    int64_t _overlap;
};

///
/// @brief 写入缓冲区参数结构体
/// @details 管理写入过程中需要的各种参数对象
///
struct WriterBufferParas {
    // 构建参数智能指针
    QSharedPointer<BPPBUILDPARAMETERS> _bppParaPtr = nullptr;
    // 客户参数智能指针
    QSharedPointer<BPCUSTOMERPARAS> _bpcParaPtr = nullptr;
    // 扩展参数智能指针
    QSharedPointer<QJsonParsing> _extendedParaPtr = nullptr;
    // 拼接模块智能指针
    QSharedPointer<SLMSplicingModule> _splicingPtr = nullptr;
    // 扩展JSON对象
    QJsonObject _extendObj;

    ///
    /// @brief 构造函数
    /// @details 初始化所有智能指针成员
    ///
    WriterBufferParas() {
        _bppParaPtr = QSharedPointer<BPPBUILDPARAMETERS>(new BPPBUILDPARAMETERS);
        _bpcParaPtr = QSharedPointer<BPCUSTOMERPARAS>(new BPCUSTOMERPARAS);
        _extendedParaPtr = QSharedPointer<QJsonParsing>(new QJsonParsing);
        _splicingPtr = QSharedPointer<SLMSplicingModule>(new SLMSplicingModule);
        _extendObj = QJsonObject();
    }

    ///
    /// @brief 获取扩展参数值
    /// @param name 参数名
    /// @param defVal 默认值
    /// @return 参数值
    ///
    template<typename T>
    T getExtendedValue(const QString &name, const T &defVal = T()) const {
        return _extendedParaPtr->getValue(name, defVal);
    }

    ///
    /// @brief 检查扩展参数是否存在
    /// @param key 参数键名
    /// @return 是否存在
    ///
    bool extendedContains(const QString &key) {
        return _extendedParaPtr->keys().contains(key);
    }
};


struct PARAWRITEBUFF {
    QFile gFile;
    QSharedPointer<WriterBufferParas> _writerBufferParas;
    QList<QList<UFILEDATAPTR>> gUFileData;

    template<typename T>
    T getExtendedValue(const QString &name, const T &defVal = T()) const {
        return _writerBufferParas->_extendedParaPtr->getValue(name, defVal);
    }

    ///
    /// @brief 创建U文件数据结构
    /// @return 是否成功创建文件
    /// @details 实现步骤:
    ///   1. 读取拼接配置参数
    ///   2. 同步平台参数
    ///   3. 创建扫描器数据结构
    ///   4. 准备文件写入
    ///
    bool createUFileData() 
    {
        auto *_writeBuff = this;
        Q_UNUSED(_writeBuff);

        // 读取拼接配置
        if(_writerBufferParas->extendedContains("Splicing/nNumber_SplicingScanner")) {
            BpcParas->nNumber_SplicingScanner = getExtendedValue("Splicing/nNumber_SplicingScanner", 1);
        }
        if(_writerBufferParas->extendedContains("Splicing/nUseSplicingMode")) {
            BppParas->sGeneralPara.nUseSplicingMode = getExtendedValue("Splicing/nUseSplicingMode", 1);
        }

        // 同步平台参数
        if(0 == BpcParas->nNumber_SplicingScanner)
        {
            BpcParas->nPlatformType = BppParas->sPlatformPara.nPlatformType;
            BpcParas->nPlatWidthX = BppParas->sPlatformPara.nPlatWidthX;
            BpcParas->nPlatWidthY = BppParas->sPlatformPara.nPlatWidthY;
            BpcParas->nNumber_SplicingScanner = BppParas->sGeneralPara.nNumber_SplicingScanner;
            
            // 复制拼接参数
            for(int iIndex = 0; iIndex < 3; ++ iIndex)
            {
                BpcParas->fSplicingPos[iIndex] = BppParas->sGeneralPara.fSplicingPos[iIndex];
                BpcParas->fSplicingLength[iIndex] = BppParas->sGeneralPara.fSplicingLength[iIndex];
            }
        }

        // 确保扫描器数量有效
        if(BpcParas->nNumber_SplicingScanner < 1) BpcParas->nNumber_SplicingScanner = 1;

        // 创建数据结构
        auto maxScannerCnt = quint8(qMax(BpcParas->nNumber_SplicingScanner, BpcParas->nScannerNumber));
        for(quint8 iScanner = 0; iScanner < maxScannerCnt; ++ iScanner) {
            QList<UFILEDATAPTR> listFileData;
            for(quint8 iBeam = 0; iBeam < quint8(BppParas->sGeneralPara.nNumber_Beam); ++ iBeam) {
                listFileData << UFILEDATAPTR(new UFILEDATA());
            }
            gUFileData << listFileData;
        }

        // 准备文件写入
        if(gFile.exists()) gFile.remove();
        if(gFile.open(QIODevice::ReadWrite)) return true;
        return false;
    }

    void clearFileDataList() {
        while(gUFileData.size()) {
            gUFileData.takeFirst().clear();
        }
    }
    void appendFileData(UFFWRITEDATA &mUFileData, const int &scanner = 0, const int &beam = 0) {
        if (scanner >= gUFileData.size())
        {
            qDebug() << "appendFileData Scanner Error" << scanner << gUFileData.size();
            return;
        }
        if (beam >= gUFileData.at(scanner).size())
        {
            qDebug() << "appendFileData Beam Error" << beam << gUFileData.at(scanner).size();
            return;
        }
        if(mUFileData.listSLines.size() < 1) return;
        QMutexLocker locker(&gUFileData[scanner][beam]->gLocker_UFD);
        gUFileData[scanner][beam]->gListUFileData << mUFileData;
    }

    inline bool isSolidSplicing() {
        auto *_writeBuff = this;
        Q_UNUSED(_writeBuff);
        return ((BpcParas->nNumber_SplicingScanner > 1) && ((BppParas->sGeneralPara.nUseSplicingMode & 0x1))
                && !getExtendedValue<int>("Splicing/nScanRangeMode", 0));
    }
    inline bool isSupportSplicing() {
        auto *_writeBuff = this;
        Q_UNUSED(_writeBuff);
        return (BpcParas->nNumber_SplicingScanner > 1 && !getExtendedValue<int>("Splicing/nScanRangeMode", 0));
    }

    PARAWRITEBUFF() {
        clearFileDataList();
    }
    ~PARAWRITEBUFF() {
        clearFileDataList();
        if(gFile.isOpen()) gFile.close();
    }
};

typedef struct __NORMALLINE {
    int64_t x1;
    int64_t y1;
    int64_t x2;
    int64_t y2;
    __NORMALLINE(int64_t in_x1 = -1, int64_t in_y1 = -1, int64_t in_x2 = -1, int64_t in_y2 = -1) {
        x1 = in_x1;
        y1 = in_y1;
        x2 = in_x2;
        y2 = in_y2;
    }
    bool operator ==(const __NORMALLINE &line) const {
        return (this->x1 == line.x1 && this->y1 == line.y1
                && this->x2 == line.x2 && this->y2 == line.y2);
    }
}LINENORMAL;

struct SplicingArea {
    Path pathArea;
    int64_t nSplicingPosLeft;
    int64_t nSplicingPosRight;

    int64_t nLastX = -1;
    int64_t nLastY = -1;
};

struct SplicingLine {
    int64_t nSplicingPos;
    QVector<LINENORMAL> vecLine;
};

struct SOLIDPATH {
    qint8 nLayerUsed[11];
    CLISOLIDINFO sLayerDatas[11];
    Paths nullPaths;
    Paths curPaths;
    Paths *lpPath_Cur;
    Paths *lpPath_Dw[5];
    Paths *lpPath_Up[5];
    Paths paths_smallGaps;
    Paths paths_smallHoles;
    Paths paths_exceptHolesAndGaps;

    void clearAllPathPointer() {
        lpPath_Cur = nullptr;
        for(int iSur = 0; iSur < 5; iSur ++) {
            lpPath_Dw[iSur] = nullptr;
            lpPath_Up[iSur] = nullptr;
        }
    }
};

inline QDebug operator<<(QDebug debug, const IntPoint &p)
{
    debug << "(" << p.X << ", " << p.Y << ")";
    return debug;
}

inline QDebug operator<<(QDebug debug, const IntRect &p)
{
    debug << "(" << p.left << ", " << p.right << ", " << p.top << ", " << p.bottom << ")";
    return debug;
}

inline QDebug operator<<(QDebug debug, const Path &path)
{
    debug = debug.nospace();
    debug << "Path " << path.size() << " { ";
    for(const auto &coor : path)
    {
        debug << "(" << coor.X << ", " << coor.Y << "), ";
    }
    debug << "}\n";
    return debug;
}

inline QDebug operator<<(QDebug debug, const DoublePoint &p)
{
    debug << "(" << p.X << ", " << p.Y << ")";
    return debug;
}

#include <QElapsedTimer>

#define RunTimerReset() RunElapsedTimer::getInterface()->resetTimer()
#define RunTimerRecord(msg) RunElapsedTimer::getInterface()->runRecord(msg)

class RunElapsedTimer {
public:
    static RunElapsedTimer *getInterface() {
        static RunElapsedTimer runTimer;
        return &runTimer;
    }
    void resetTimer() {
        timer.start();
        nLastTime = 0;
    }
    void runRecord(const QString &msg) {
        auto nCurRunTime = timer.elapsed();
        qDebug() << nCurRunTime << "\t" <<  nCurRunTime - nLastTime << "\t" << msg.toLocal8Bit().data();
        nLastTime = nCurRunTime;
    }

protected:
    RunElapsedTimer() = default;

private:
    QElapsedTimer timer;
    qint64 nLastTime = 0;
};

class PathsInfo
{
public:
    PathsInfo() = default;
    static QString getPathsInfo(const Paths &paths) {
        QString tempStr = QString(" %1:").arg(paths.size());
        for(const auto &path : paths) {
            tempStr += QString(" %1").arg(path.size());
        }
        return tempStr;
    }
};

#endif // PUBLICHEADER_H
