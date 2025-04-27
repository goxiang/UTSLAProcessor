#ifndef LATTICEINTERFACE_H
#define LATTICEINTERFACE_H

#include "qjsonparsing.h"
#include "clipper.hpp"

struct LatticeLayerInfo;
///
/// ! @coreclass{LatticeInterface}
/// 连接外部系统和内部晶格处理模块的桥梁，为SLA打印提供统一的晶格处理入口
///
class LatticeInterface
{
public:
    LatticeInterface();

    void initLatticeParas(const QJsonParsing *);
    bool calcLayerLattic(ClipperLib::Paths &, const ClipperLib::Paths &,
                         const int &, ClipperLib::Paths &, LatticeLayerInfo &);

private:
    struct Priv;
    QSharedPointer<Priv> d = nullptr;
};

struct LatticeInfo {
    int _type = 0;
    double _area = 0.0;
    int _centerX = 0;
    int _centerY = 0;

    LatticeInfo() = default;
    LatticeInfo(const int &type, const double &area, const int &cx, const int &cy) {
        _type = type;
        _area = area;
        _centerX = cx;
        _centerY = cy;
    }
};

struct LatticePathInfo {
    int _type = 0;
    ClipperLib::Path _path;
    int _centerX = 0;
    int _centerY = 0;
};

struct LatticeLayerInfo {
    QVector<LatticeInfo> _latticeInfo;
    QMap<int, LatticePathInfo> _latticePathInfo;
    void initialize() {
        _latticeInfo.clear();
        _latticePathInfo.clear();
    }
};

inline bool operator==(const LatticeInfo &lhs, const LatticeInfo &rhs) {
    return fabs(lhs._area - rhs._area) < 100000 &&
           fabs(lhs._centerX - rhs._centerX) < 2 &&
           fabs(lhs._centerY - rhs._centerY) < 2;
}

inline uint qHash(LatticeInfo key, uint seed = 0)
{
    return qHash(key._centerX, seed) ^
           qHash(key._centerY << 15, seed);
}

#include <QThread>
#include <QSemaphore>
///
/// @brief 高优先级线程执行器类
/// @details 继承自QThread,用于执行高优先级任务
///
class ThreadRunner : public QThread
{
public:
    /// @brief 构造函数
    /// @param parent 父对象指针
    /// @details 实现步骤:
    ///   1. 初始化信号量
    ///   2. 设置线程最高优先级
    explicit ThreadRunner(QObject *parent = nullptr) : QThread(parent) {
        _semaphore.release();
        this->setPriority(QThread::HighestPriority);
    }

    /// @brief 析构函数
    /// @details 实现步骤:
    ///   1. 退出线程
    ///   2. 等待线程结束
    ~ThreadRunner() {
        this->quit();
        this->wait();
    }

    /// @brief 执行任务
    /// @param func 待执行的函数对象
    /// @details 实现步骤:
    ///   1. 保存任务函数
    ///   2. 获取信号量
    ///   3. 启动线程
    void runTask(const std::function<void()> &func) {
        _func = std::move(func);
        _semaphore.acquire();
        start();
    }

    /// @brief 等待任务完成
    /// @details 实现步骤:
    ///   1. 获取信号量(阻塞等待)
    ///   2. 释放信号量
    void waitForFinished() {
        _semaphore.acquire();
        _semaphore.release();
    }

protected:
    /// @brief 线程执行函数
    /// @details 实现步骤:
    ///   1. 执行任务函数
    ///   2. 释放信号量
    void run() override { 
        _func(); 
        _semaphore.release(); 
    }

private:
    QSemaphore _semaphore;                    // 同步信号量
    std::function<void()> _func = [](){};     // 任务函数
};


#endif // LATTICEINTERFACE_H
