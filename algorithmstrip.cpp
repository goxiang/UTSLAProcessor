#include "algorithmstrip.h"
#include "bpccommon.h"
#include "publicheader.h"

/// 根据JSON配置创建条带
/// @brief 解析JSON参数并调用主要条带创建函数
/// @param jsonObj JSON配置对象,包含条带高度和间距参数
/// @param platWidth 平台宽度
/// @param platHeight 平台高度
/// @details 实现步骤:
/// 1. 从JSON中读取条带高度和重叠宽度参数
/// 2. 将参数转换为内部精度单位
/// 3. 调用主条带创建函数
void AlgorithmStrip::createStrips(const QJsonObject &jsonObj, const int &platWidth, const int &platHeight)
{
    // 从JSON读取条带高度并转换为内部单位
    int nStripHei = qRound(jsonObj.value("fStripHei").toDouble() * UNITSPRECISION);

    // 从JSON读取重叠宽度并转换为内部单位 
    int nOverlapWid = qRound(jsonObj.value("fSpacing").toDouble() * UNITSPRECISION);

    // 调用主要条带创建函数
    createStrips(nStripHei, platWidth, platHeight, nOverlapWid);
}



/// 创建平行条带路径
/// @brief 根据给定参数在平台上生成等间距平行条带
/// @param nHei 条带高度
/// @param platWidth 平台宽度
/// @param platHeight 平台高度 
/// @param overlapWid 重叠宽度
/// @details 实现步骤:
/// 1. 计算扩展边界
/// 2. 计算中心偏移
/// 3. 生成平行条带路径
void AlgorithmStrip::createStrips(const int &nHei, const int &platWidth, const int &platHeight, const int &overlapWid)
{
    // 计算扩展边界,扩展25%以确保覆盖
    int nMaxEdgeLen = (platWidth > platHeight) ? platWidth : platHeight;
    int nMinX = - qRound(nMaxEdgeLen * 0.25);
    int nMinY = nMinX;
    int nMaxX = platWidth - nMinX;
    int nMaxY = platHeight - nMinY;

    // 计算中心偏移量
    nOffsetX = platWidth >> 1;  // 除以2
    nOffsetY = platHeight >> 1;

    // 应用中心偏移
    nMinX -= nOffsetX;
    nMaxX -= nOffsetX;
    nMinY -= nOffsetY;
    nMaxY -= nOffsetY;

    // 清空并生成平行条带路径
    pathsTemp.clear();
    for(int iY = nMinY; iY <= nMaxY; iY += nHei + overlapWid)
    {
        // 创建矩形条带路径
        Path path;
        path << IntPoint(nMinX, iY) << IntPoint(nMaxX, iY)
             << IntPoint(nMaxX, iY + nHei) << IntPoint(nMinX, iY + nHei);
        pathsTemp << path;
    }
}


/// 获取指定角度和风向的条带路径列表
/// @brief 根据给定角度和风向计算旋转后的条带路径
/// @param angle 条带角度
/// @param hatchingAngle 输出参数,填充角度
/// @param windDirection 风向(0-3)
/// @return 旋转后的条带路径集合
/// @details 实现步骤:
/// 1. 标准化条带角度到[0,360)
/// 2. 根据风向调整条带和填充角度
/// 3. 计算旋转矩阵
/// 4. 应用旋转变换生成结果路径
Paths AlgorithmStrip::getStripList(const double &angle, double &hatchingAngle, const int &windDirection)
{
    // 标准化条带角度到[0,360)范围
    double fStripAngle = angle;
    while(fStripAngle < 0) fStripAngle += 360;
    while(fStripAngle >= 360) fStripAngle -= 360;

    // 根据风向调整条带和填充角度
    switch(windDirection) {
    case 0: {
        // 北风 [0 90) [270 360)
        if(fStripAngle >= 90 && fStripAngle < 270) fStripAngle += 180;
        hatchingAngle = fStripAngle + 90;
        while(hatchingAngle >= 360) hatchingAngle -= 360;
        if(hatchingAngle >= 90 && hatchingAngle < 180) hatchingAngle += 180;
        else if(hatchingAngle >= 180 && hatchingAngle < 270) hatchingAngle -= 180;
    }
    break;
    case 1: {
        // 东风 [180 360)
        if(fStripAngle >= 0 && fStripAngle < 180) fStripAngle += 180;
        hatchingAngle = fStripAngle + 90;
        while(hatchingAngle >= 360) hatchingAngle -= 360;
        if(hatchingAngle < 180) hatchingAngle += 180;
    }
    break;
    case 2: {
        // 南风 [90 270)
        if(fStripAngle >= 0 && fStripAngle < 90) fStripAngle += 180;
        if(fStripAngle >= 270 && fStripAngle < 360) fStripAngle -= 180;
        hatchingAngle = fStripAngle + 90;
        while(hatchingAngle > 360) hatchingAngle -= 360;
        if(hatchingAngle > 270) hatchingAngle -= 180;
    }
    break;
    case 3: {
        // 西风 [0 180)
        if(fStripAngle >= 180 && fStripAngle < 360) fStripAngle -= 180;
        hatchingAngle = fStripAngle + 90;
        while(hatchingAngle >= 360) hatchingAngle -= 360;
        if(hatchingAngle >= 180) hatchingAngle -= 180;
    }
    break;
    }

    // 标准化最终填充角度
    if(hatchingAngle >= 360) hatchingAngle -= 360;
    if(hatchingAngle < 0) hatchingAngle += 360;

    // 计算旋转矩阵
    double fArc = fStripAngle * DEF_PI / 180;
    double m11 = cos(fArc);
    double m21 = sin(fArc);
    double m12 = -m21;
    double m22 = m11;

    // 应用旋转变换生成结果路径
    Paths resultPath;
    for(const auto &path : pathsTemp)
    {
        Path temPath;
        for(const auto &coor : path)
        {
            temPath << IntPoint(qRound(m11 * coor.X + m12 * coor.Y + nOffsetX),
                               qRound(m21 * coor.X + m22 * coor.Y + nOffsetX));
        }
        resultPath << temPath;
    }
    return resultPath;
}
