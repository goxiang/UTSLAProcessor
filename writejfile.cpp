#include "writejfile.h"
#include "publicheader.h"
#include "sljfilewriter.h"

#include <QDebug>

WriteJFile::WriteJFile(QObject *parent) :
    QThread(parent)
{
}

void WriteJFile::setJFileLayerInfo(JFileDef::JFileLayerInfo *layerInfo)
{
    jFLayerInfo = layerInfo;
}

///
/// @brief 设置文件写入状态
/// @param nStatus 状态值(UFFWRITE_BEGIN:开始写入, UFFWRITE_END:结束写入)
/// @details 实现步骤:
///   1. 更新文件写入状态标志
///
void WriteJFile::setUFFStatus(int nStatus)
{
    // 设置文件写入状态
    m_nUFDataStatus = nStatus;
}


///
/// @brief 设置写入缓冲区
/// @param lpParaWriteBuff 写入缓冲区指针
/// @details 实现步骤:
///   1. 保存缓冲区指针
///   2. 用于后续写入操作
///
void WriteJFile::setParaWriteBuff(PARAWRITEBUFF *lpParaWriteBuff)
{
    // 设置写入缓冲区指针
    _writeBuff = lpParaWriteBuff;
}


void WriteJFile::run()
{
    m_bRunning = true;
    for(int iScanner = 0; iScanner < BpcParas->nScannerNumber; ++ iScanner)
    {
        int nCurBeamIndex = BppParas->sGeneralPara.nNumber_Beam - 1;
        bool bCurBeamWrited = false;
        bool bCurScannerIndexWrited = false;
        while(m_bRunning)
        {
            UFFWRITEDATA mUFileData;

            QMutex *mutex = &_writeBuff->gUFileData[iScanner][nCurBeamIndex]->gLocker_UFD;
            mutex->lock();
            if(_writeBuff->gUFileData[iScanner][nCurBeamIndex]->gListUFileData.size())
            {
                mUFileData = _writeBuff->gUFileData[iScanner][nCurBeamIndex]->gListUFileData.takeFirst();
            }
            else
            {
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

            if(-1 != mUFileData.nMode_Section)
            {
                if(false == bCurScannerIndexWrited) bCurScannerIndexWrited = true;
                if(false == bCurBeamWrited) bCurBeamWrited = true;
                SLJFileWriter::writeDataBlock(iScanner, mUFileData.nMode_Section - SECTION_POLYGON, &_writeBuff->gFile,
                                              mUFileData.listSLines, jFLayerInfo);
            }
        }
    }
}

