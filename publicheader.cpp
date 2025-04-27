#include "publicheader.h"

#include <QThread>

void writeData(QFile *, void *, const int &);
///
/// @brief 写入指定长度的数据到文件
/// @param lpFile 文件指针
/// @param data 数据指针
/// @param nLen 数据长度
/// @details 实现步骤:
///   1. 将数据指针转换为char*类型
///   2. 写入指定长度的数据
///
void writeData(QFile *lpFile, void *data, const int &nLen)
{
    lpFile->write(static_cast<char *>(data), nLen);
}

///
/// @brief 写入8位整数到文件
/// @param lpFile 文件指针
/// @param nData 8位整数数据
/// @details 实现步骤:
///   1. 调用writeData写入1字节数据
///
void writeData8(QFile *lpFile, qint8 nData)
{
    writeData(lpFile, &nData, 1);
}


void writeData32(QFile *lpFile, qint32 nData)
{
    writeData(lpFile, &nData, 4);
}

///
/// @brief 将扫描线数据写入文件
/// @param lpFile 输出文件指针
/// @param listSLines 扫描线数据列表
/// @details 实现步骤:
///   1. 遍历扫描线列表
///   2. 区分跳转和标记线段
///   3. 对标记线段进行优化存储
///   4. 按不同格式写入文件
///
void writeScanLines(QFile *lpFile, QVector<SCANLINE> &listSLines)
{
    // 记录上一个点的坐标
    int nLastX = -1, nLastY = -1;
    int nLineCnt = listSLines.count();

    if(nLineCnt)
    {
        // 存储标记线段
        QVector<SCANLINE> listMarkSLine;

        // 遍历所有扫描线
        for(int iLCnt = 0; iLCnt < nLineCnt; iLCnt ++)
        {
            // 处理跳转线段
            if(SECTION_SCANTYPE_JUMP == listSLines.at(iLCnt).nLineType)
            {
                // 检查是否为新位置
                if(nLastX != listSLines.at(iLCnt).nX || nLastY != listSLines.at(iLCnt).nY)
                {
                    // 先处理已存储的标记线段
                    int nMarkSLineCnt = listMarkSLine.count();
                    if(nMarkSLineCnt)
                    {
                        // 标记线段数量大于4时使用循环格式
                        if(nMarkSLineCnt > 4)
                        {
                            writeData8(lpFile, SECTION_SCANTYPE_MARKLOOP);
                            writeData32(lpFile, nMarkSLineCnt);
                            for(int iMark = 0; iMark < nMarkSLineCnt; iMark ++)
                            {
                                writeData32(lpFile, listMarkSLine.at(iMark).nX);
                                writeData32(lpFile, listMarkSLine.at(iMark).nY);
                            }
                        }
                        // 少量标记线段直接写入
                        else
                        {
                            for(int iMark = 0; iMark < nMarkSLineCnt; iMark ++)
                            {
                                writeData8(lpFile, listMarkSLine.at(iMark).nLineType);
                                writeData32(lpFile, listMarkSLine.at(iMark).nX);
                                writeData32(lpFile, listMarkSLine.at(iMark).nY);
                            }
                        }
                        listMarkSLine.clear();
                    }

                    // 写入跳转线段
                    writeData8(lpFile, listSLines.at(iLCnt).nLineType);
                    writeData32(lpFile, listSLines.at(iLCnt).nX);
                    writeData32(lpFile, listSLines.at(iLCnt).nY);
                    nLastX = listSLines.at(iLCnt).nX;
                    nLastY = listSLines.at(iLCnt).nY;
                }
            }
            // 处理标记线段
            else if(SECTION_SCANTYPE_MARK == listSLines.at(iLCnt).nLineType)
            {
                // 检查是否为新位置
                if(nLastX != listSLines.at(iLCnt).nX || nLastY != listSLines.at(iLCnt).nY)
                {
                    // 添加到标记线段列表
                    listMarkSLine << listSLines.at(iLCnt);
                    nLastX = listSLines.at(iLCnt).nX;
                    nLastY = listSLines.at(iLCnt).nY;
                }
            }
        }

        // 处理剩余的标记线段
        int nMarkSLineCnt = listMarkSLine.count();
        if(nMarkSLineCnt)
        {
            // 使用循环格式或直接写入
            if(nMarkSLineCnt > 4)
            {
                writeData8(lpFile, SECTION_SCANTYPE_MARKLOOP);
                writeData32(lpFile, nMarkSLineCnt);
                for(int iMark = 0; iMark < nMarkSLineCnt; iMark ++)
                {
                    writeData32(lpFile, listMarkSLine.at(iMark).nX);
                    writeData32(lpFile, listMarkSLine.at(iMark).nY);
                }
            }
            else
            {
                for(int iMark = 0; iMark < nMarkSLineCnt; iMark ++)
                {
                    writeData8(lpFile, listMarkSLine.at(iMark).nLineType);
                    writeData32(lpFile, listMarkSLine.at(iMark).nX);
                    writeData32(lpFile, listMarkSLine.at(iMark).nY);
                }
            }
            listMarkSLine.clear();
        }
        listSLines.clear();
    }
}
