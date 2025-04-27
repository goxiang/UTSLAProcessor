#include "algorithmchecker.h"
#include "bpccommon.h"

/// 根据JSON配置创建棋盘格
/// @brief 解析JSON参数并创建重叠棋盘格
/// @param jsonObj JSON配置对象,包含棋盘格参数
/// @param platWidth 平台宽度
/// @param platHeight 平台高度
/// @details 实现步骤:
/// 1. 从JSON读取棋盘格宽度、高度、重叠宽度等参数
/// 2. 将参数转换为内部精度单位
/// 3. 标准化旋转角度到[0,360]范围
/// 4. 调用重叠棋盘格创建函数
void AlgorithmChecker::createCheckers(const QJsonObject &jsonObj, const int &platWidth, const int &platHeight)
{
    // 从JSON读取参数并转换单位
    int nWid = qRound(jsonObj.value("fWid").toDouble() * UNITSPRECISION);
    int nHei = qRound(jsonObj.value("fHei").toDouble() * UNITSPRECISION);
    int nOverlapWid = qRound(jsonObj.value("fOverlapWid").toDouble() * UNITSPRECISION);
    nOffsetChecker = qRound(jsonObj.value("fOffsetChecker").toDouble() * UNITSPRECISION);
    nScanCheckerBorder = jsonObj.value("nScanCheckerBorder").toInt();
    fRotateAngle = jsonObj.value("fRotateAngle").toDouble();

    // 标准化旋转角度到[0,360]范围
    while(fRotateAngle < 0) fRotateAngle += 360;
    while(fRotateAngle > 360) fRotateAngle -= 360;

    // 调用重叠棋盘格创建函数
    createOverlapCheckers(nWid, nHei, platWidth, platHeight, nOverlapWid, nOffsetChecker);
}

/// 创建重叠棋盘格路径
/// @brief 根据给定参数生成带重叠的棋盘格路径
/// @param nWid 棋盘格宽度
/// @param nHei 棋盘格高度
/// @param platWidth 平台宽度
/// @param platHeight 平台高度
/// @param overlapWid 重叠宽度
/// @param offsetChecker 偏移量
/// @details 实现步骤:
/// 1. 计算棋盘格行列数
/// 2. 生成四组基本棋盘格路径
/// 3. 如果有偏移量,生成四组偏移棋盘格路径
void AlgorithmChecker::createOverlapCheckers(const int &nWid, const int &nHei, const int &platWidth, const int &platHeight,
                                             const int &overlapWid, const int &offsetChecker)
{
    Paths pathTemp;
    // 计算棋盘格行列数
    int nWidCnt = int(ceil(double(platWidth + offsetChecker) / nWid) + 0.001);
    int nHeiCnt = int(ceil(double(platHeight + offsetChecker) / nHei) + 0.001);

    // 生成第一组基本棋盘格(偶数行偶数列)
    pathTemp.clear();
    for(int iWid = 0; iWid < nWidCnt; iWid += 2)
    {
        for(int iHei = 0; iHei < nHeiCnt; iHei += 2)
        {
            Path path;
            path << IntPoint(iWid * nWid, iHei * nHei)
                 << IntPoint(iWid * nWid + nWid, iHei * nHei)
                 << IntPoint(iWid * nWid + nWid, iHei * nHei + nHei)
                 << IntPoint(iWid * nWid, iHei * nHei + nHei);
            pathTemp << path;
        }
    }
    checkerList << pathTemp;

    // 生成第二组基本棋盘格(偶数行奇数列,带重叠)
    pathTemp.clear();
    for(int iWid = 0; iWid < nWidCnt; iWid += 2)
    {
        for(int iHei = 1; iHei < nHeiCnt; iHei += 2)
        {
            Path path;
            path << IntPoint(iWid * nWid, iHei * nHei)
                 << IntPoint(iWid * nWid + nWid + overlapWid, iHei * nHei)
                 << IntPoint(iWid * nWid + nWid + overlapWid, iHei * nHei + nHei + overlapWid)
                 << IntPoint(iWid * nWid, iHei * nHei + nHei + overlapWid);
            pathTemp << path;
        }
    }
    checkerList << pathTemp;

    // 生成第三组基本棋盘格(奇数行奇数列)
    pathTemp.clear();
    for(int iWid = 1; iWid < nWidCnt; iWid += 2)
    {
        for(int iHei = 1; iHei < nHeiCnt; iHei += 2)
        {
            Path path;
            path << IntPoint(iWid * nWid, iHei * nHei)
                 << IntPoint(iWid * nWid + nWid, iHei * nHei)
                 << IntPoint(iWid * nWid + nWid, iHei * nHei + nHei)
                 << IntPoint(iWid * nWid, iHei * nHei + nHei);
            pathTemp << path;
        }
    }
    checkerList << pathTemp;

    // 生成第四组基本棋盘格(奇数行偶数列,带重叠)
    pathTemp.clear();
    for(int iWid = 1; iWid < nWidCnt; iWid += 2)
    {
        for(int iHei = 0; iHei < nHeiCnt; iHei += 2)
        {
            Path path;
            path << IntPoint(iWid * nWid, iHei * nHei)
                 << IntPoint(iWid * nWid + nWid + overlapWid, iHei * nHei)
                 << IntPoint(iWid * nWid + nWid + overlapWid, iHei * nHei + nHei + overlapWid)
                 << IntPoint(iWid * nWid, iHei * nHei + nHei + overlapWid);
            pathTemp << path;
        }
    }
    checkerList << pathTemp;

    // 如果有偏移量,生成四组偏移棋盘格路径
    if(offsetChecker)
    {
        // 生成四组偏移棋盘格,结构与基本棋盘格相同
        // 但坐标都减去offsetChecker
        pathTemp.clear();
        for(int iWid = 0; iWid < nWidCnt; iWid += 2)
        {
            for(int iHei = 0; iHei < nHeiCnt; iHei += 2)
            {
                Path path;
                path << IntPoint(iWid * nWid - offsetChecker, iHei * nHei - offsetChecker)
                     << IntPoint(iWid * nWid + nWid - offsetChecker, iHei * nHei - offsetChecker)
                     << IntPoint(iWid * nWid + nWid - offsetChecker, iHei * nHei + nHei - offsetChecker)
                     << IntPoint(iWid * nWid - offsetChecker, iHei * nHei + nHei - offsetChecker);
                pathTemp << path;
            }
        }
        checkerListOffset << pathTemp;

        pathTemp.clear();
        for(int iWid = 0; iWid < nWidCnt; iWid += 2)
        {
            for(int iHei = 1; iHei < nHeiCnt; iHei += 2)
            {
                Path path;
                path << IntPoint(iWid * nWid - offsetChecker, iHei * nHei - offsetChecker)
                     << IntPoint(iWid * nWid + nWid + overlapWid - offsetChecker, iHei * nHei - offsetChecker)
                     << IntPoint(iWid * nWid + nWid + overlapWid - offsetChecker, iHei * nHei + nHei + overlapWid - offsetChecker)
                     << IntPoint(iWid * nWid - offsetChecker, iHei * nHei + nHei + overlapWid - offsetChecker);
                pathTemp << path;
            }
        }
        checkerListOffset << pathTemp;

        pathTemp.clear();
        for(int iWid = 1; iWid < nWidCnt; iWid += 2)
        {
            for(int iHei = 1; iHei < nHeiCnt; iHei += 2)
            {
                Path path;
                path << IntPoint(iWid * nWid - offsetChecker, iHei * nHei - offsetChecker)
                     << IntPoint(iWid * nWid + nWid - offsetChecker, iHei * nHei - offsetChecker)
                     << IntPoint(iWid * nWid + nWid - offsetChecker, iHei * nHei + nHei - offsetChecker)
                     << IntPoint(iWid * nWid - offsetChecker, iHei * nHei + nHei - offsetChecker);
                pathTemp << path;
            }
        }
        checkerListOffset << pathTemp;

        pathTemp.clear();
        for(int iWid = 1; iWid < nWidCnt; iWid += 2)
        {
            for(int iHei = 0; iHei < nHeiCnt; iHei += 2)
            {
                Path path;
                path << IntPoint(iWid * nWid - offsetChecker, iHei * nHei - offsetChecker)
                     << IntPoint(iWid * nWid + nWid + overlapWid - offsetChecker, iHei * nHei - offsetChecker)
                     << IntPoint(iWid * nWid + nWid + overlapWid - offsetChecker, iHei * nHei + nHei + overlapWid - offsetChecker)
                     << IntPoint(iWid * nWid - offsetChecker, iHei * nHei + nHei + overlapWid - offsetChecker);
                pathTemp << path;
            }
        }
        checkerListOffset << pathTemp;
    }
}
