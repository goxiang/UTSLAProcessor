#include "writeuff.h"
#include "publicheader.h"

#include <QDebug>

///
/// @brief UFF文件写入线程构造函数
/// @param parent 父对象指针
/// @details 继承自QThread的线程类构造
///
WriteUFF::WriteUFF(QObject *parent) : QThread(parent)
{
}

///
/// @brief 设置UFF写入状态
/// @param nStatus 状态值
/// @details 更新内部状态标志
///
void WriteUFF::setUFFStatus(int nStatus)
{
    m_nUFDataStatus = nStatus;
}

///
/// @brief 设置写入缓冲区
/// @param lpParaWriteBuff 缓冲区指针
/// @details 关联写入缓冲区对象
///
void WriteUFF::setParaWriteBuff(PARAWRITEBUFF *lpParaWriteBuff)
{
    _writeBuff = lpParaWriteBuff;
}

///
/// @brief UFF文件写入线程执行函数
/// @details 实现步骤:
///   1. 遍历扫描器
///   2. 处理每个光束的数据
///   3. 写入扫描器和光束索引
///   4. 写入零件和扫描参数
///   5. 写入扫描线数据
///
void WriteUFF::run()
{
    // 设置运行标志
    m_bRunning = true;

    // 遍历所有扫描器
    for(int iScanner = 0; iScanner < BpcParas->nNumber_SplicingScanner; ++ iScanner)
    {
        // 初始化光束索引和状态
        int nCurBeamIndex = BppParas->sGeneralPara.nNumber_Beam - 1;
        bool bCurBeamWrited = false;
        bool bCurScannerIndexWrited = false;

        // 处理当前扫描器的数据
        while(m_bRunning)
        {
            UFFWRITEDATA mUFileData;

            // 获取写入数据
            QMutex *mutex = &_writeBuff->gUFileData[iScanner][nCurBeamIndex]->gLocker_UFD;
            mutex->lock();
            if(_writeBuff->gUFileData[iScanner][nCurBeamIndex]->gListUFileData.size())
            {
                mUFileData = _writeBuff->gUFileData[iScanner][nCurBeamIndex]->gListUFileData.takeFirst();
            }
            else
            {
                // 检查是否结束当前光束处理
                if(UFFWRITE_END == m_nUFDataStatus)
                {
                    if(-- nCurBeamIndex < 0)
                    {
                        mutex->unlock();
                        break;
                    }
                    bCurBeamWrited = false;
                }
            }
            mutex->unlock();

            // 写入数据段
            if(-1 != mUFileData.nMode_Section)
            {
                // 写入扫描器索引
                if(false == bCurScannerIndexWrited)
                {
                    bCurScannerIndexWrited = true;
                    writeData8(&_writeBuff->gFile, SECTION_CURRENTSCANNERINDEX);
                    writeData32(&_writeBuff->gFile, iScanner);
                }

                // 写入光束索引
                if(false == bCurBeamWrited)
                {
                    bCurBeamWrited = true;
                    writeData8(&_writeBuff->gFile, SECTION_CURRENTBEAMINDEX);
                    writeData32(&_writeBuff->gFile, nCurBeamIndex);
                }

                // 写入零件索引
                writeData8(&_writeBuff->gFile, SECTION_PARTINDEX);
                writeData32(&_writeBuff->gFile, mUFileData.nPartIndex);

                // 写入模式信息
                writeData8(&_writeBuff->gFile, mUFileData.nMode_Section);
                writeData8(&_writeBuff->gFile, mUFileData.nMode_Coor);

                // 写入激光功率
                writeData8(&_writeBuff->gFile, SECTION_LASERPOWER);
                writeData32(&_writeBuff->gFile, mUFileData.nLaserPower);

                // 写入标记速度
                writeData8(&_writeBuff->gFile, SECTION_MARKSPEED);
                writeData32(&_writeBuff->gFile, mUFileData.nMarkSpeed);

                // 写入扫描线数据
                writeScanLines(&_writeBuff->gFile, mUFileData.listSLines);
            }
        }
    }
}
