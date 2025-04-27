#include "algorithmhatching.h"
#include <QThread>

/// 用于调试输出的运算符重载函数
// 第一个输出边界矩形的四个坐标值
// 第二个输出边界矩形指针指向的四个坐标值
// 第三个以(x,y)格式输出期望点的坐标
inline QDebug operator<<(QDebug debug, const BOUNDINGRECT &p)
{
    debug << p.minX  << " " << p.maxX  << " " << p.minY  << " "  << p.maxY;
    return debug;
}

inline QDebug operator<<(QDebug debug, BDRECTPTR p)
{
    debug << p->minX  << p->maxX << p->minY  << p->maxY;
    return debug;
}


inline QDebug operator<<(QDebug debug, EXPECTPTINFOPTR p)
{
    debug << "(" << p->X  << " " << p->Y << ")";
    return debug;
}

/// 计算填充线与轮廓的交点
/// @brief 根据给定角度计算填充线与轮廓的交点,生成填充线段
/// @param curPaths 输入轮廓路径集合
/// @param totalHLine 输出填充线段集合
/// @param nTotalLCnt 输出填充线段总数
/// @param fAngle_Hatching 填充角度
/// @param nLSpacing 填充线间距
/// @param bInner 是否内部填充
/// @details 实现步骤:
/// 1. 根据填充角度计算扫描线参数(起点、方向、步进等)
/// 2. 循环生成平行扫描线
/// 3. 计算扫描线与轮廓的交点
/// 4. 将交点配对生成填充线段
void AlgorithmHatching::calcInnerPoint(const Paths &curPaths, TOTALHATCHINGLINE &totalHLine, int &nTotalLCnt,
                                  const double &fAngle_Hatching, const int &nLSpacing, const bool &bInner)
{   
    // 计算轮廓的边界框
    BOUNDINGRECT outRC;
    QList<BDRECTPTR> listRc;
    calcLimitXY(curPaths, outRC, &listRc);

//    qDebug() << "pathInner_Temp" << fAngle_Hatching << nLSpacing << outRC << listRc;

    // // 根据填充角度计算填充线的起点、方向、步进等参数
    double k_temp = 0.0, b_temp = 0.0;
    double X1 = 0.0, Y1 = 0.0, X2 = 0.0, Y2 = 0.0;
    double fDelta_X = 0.0, fDelta_Y = 0;
    double fLimit_X = 0.0, fLimit_Y = 0.0;
    int nLimitType = 0;
    int nKMode = 0;
    int nSortType = 0;

    // 将角度转换为弧度
    double fAngle_Arc = fAngle_Hatching * DEF_PI / 180;

    // 根据不同角度范围设置扫描线参数
    // 0度填充
    if(fabs(fAngle_Hatching) < 1E-6)
    {
        X1 = 0;
        Y1 = outRC.minY;
        X2 = 5000;
        Y2 = outRC.minY;

        fDelta_X = 0;
        fDelta_Y = nLSpacing;

        fLimit_X = 6000;
        fLimit_Y = outRC.maxY;

        nLimitType = LIMITTYPE_SMALL2BIG;
        nKMode = KMODE_HORIZONTAL;
        nSortType = SORTTYPE_X;
    }
    // 0-45度填充
    else if(fAngle_Hatching < 45)
    {
        k_temp = tan(fAngle_Arc);
        b_temp = outRC.minY - k_temp * outRC.maxX;
        X1 = - 6000;
        Y1 = k_temp * X1 + b_temp;
        X2 = 0;
        Y2 = b_temp;
        fDelta_X = 0;
        fDelta_Y = nLSpacing / cos(fAngle_Arc);

        fLimit_X = 1000;
        fLimit_Y = outRC.maxY - k_temp * outRC.minX;
        nLimitType = LIMITTYPE_SMALL2BIG;
        nKMode = KMODE_NORMAL;
        nSortType = SORTTYPE_X;
    }
    // 45-90度填充
    else if(fAngle_Hatching < 90)
    {
        k_temp = tan(fAngle_Arc);
        b_temp = outRC.minY - k_temp * outRC.maxX;
        Y1 = 0;
        X1 = (Y1 - b_temp) / k_temp;
        Y2 = 6000;
        X2 = (Y2 - b_temp) / k_temp;
        fDelta_X = - nLSpacing / sin(fAngle_Arc);
        fDelta_Y = 0;

        fLimit_X = outRC.minX - outRC.maxY / k_temp;
        fLimit_Y = - 100;
        nLimitType = LIMITTYPE_BIG2SMALL;
        nKMode = KMODE_NORMAL;
        nSortType = SORTTYPE_Y;
    }
    // 90度填充
    else if(fabs(fAngle_Hatching - 90) < 1E-6)
    {
        X1 = outRC.maxX;
        Y1 = 0;
        X2 = outRC.maxX;
        Y2 = 100;
        fDelta_X = - nLSpacing;
        fDelta_Y = 0;

        fLimit_X = outRC.minX;
        fLimit_Y = - 1000;
        nLimitType = LIMITTYPE_BIG2SMALL;
        nKMode = KMODE_VERTICAL;
        nSortType = SORTTYPE_Y;
    }
    // 90-135度填充
    else if(fAngle_Hatching < 135)
    {
        k_temp = tan(fAngle_Arc);
        b_temp = outRC.maxY - k_temp * outRC.maxX;
        Y1 = - 5000;
        X1 = (Y1 - b_temp) / k_temp;
        Y2 = 0;
        X2 = (Y2 - b_temp) / k_temp;
        fDelta_X = - nLSpacing / sin(fAngle_Arc);
        fDelta_Y = 0;

        fLimit_X = outRC.minX - outRC.minY / k_temp;
        fLimit_Y = - 16000;
        nLimitType = LIMITTYPE_BIG2SMALL;
        nKMode = KMODE_NORMAL;
        nSortType = SORTTYPE_Y;
    }
    // 135-180度填充
    else if(fAngle_Hatching < 180)
    {
        k_temp = tan(fAngle_Arc);
        b_temp = outRC.maxY - k_temp * outRC.maxX;
        X1 = 0;
        Y1 = b_temp;
        X2 = - 5000;
        Y2 = k_temp * X2 + b_temp;
        fDelta_X = 0;
        fDelta_Y = nLSpacing / cos(fAngle_Arc);

        fLimit_X = - 6000;
        fLimit_Y = outRC.minY - k_temp * outRC.minX;
        nLimitType = LIMITTYPE_BIG2SMALL;
        nKMode = KMODE_NORMAL;
        nSortType = SORTTYPE_X;
    }
    // 180度填充
    else if(fabs(fAngle_Hatching - 180) < 1E-6)
    {
        X1 = 5000;
        Y1 = outRC.maxY;
        X2 = 0;
        Y2 = outRC.maxY;
        fDelta_X = 0;
        fDelta_Y = - nLSpacing;

        fLimit_X = - 100;
        fLimit_Y = outRC.minY;
        nLimitType = LIMITTYPE_BIG2SMALL;
        nKMode = KMODE_HORIZONTAL;
        nSortType = SORTTYPE_X;
    }
    // 180-225度填充
    else if(fAngle_Hatching < 225)
    {
        k_temp = tan(fAngle_Arc);
        b_temp = outRC.maxY - k_temp * outRC.minX;
        X1 = 5000;
        Y1 = k_temp * X1 + b_temp;
        X2 = 0;
        Y2 = b_temp;
        fDelta_X = 0;
        fDelta_Y = nLSpacing / cos(fAngle_Arc);

        fLimit_X = - 100;
        fLimit_Y = outRC.minY - k_temp * outRC.maxX;
        nLimitType = LIMITTYPE_BIG2SMALL;
        nKMode = KMODE_NORMAL;
        nSortType = SORTTYPE_X;
    }
    // 225-270度填充
    else if(fAngle_Hatching < 270)
    {
        k_temp = tan(fAngle_Arc);
        b_temp = outRC.maxY - k_temp * outRC.minX;
        Y1 = 0;
        X1 = (Y1 - b_temp) / k_temp;
        Y2 = - 5000;
        X2 = (Y2 - b_temp) / k_temp;
        fDelta_X = - nLSpacing / sin(fAngle_Arc);
        fDelta_Y = 0;

        fLimit_X = outRC.maxX - outRC.minY / k_temp;
        fLimit_Y = 100;
        nLimitType = LIMITTYPE_SMALL2BIG;
        nKMode = KMODE_NORMAL;
        nSortType = SORTTYPE_Y;
    }
    // 270度填充
    else if(fabs(fAngle_Hatching - 270) < 1E-6)
    {
        X1 = outRC.minX;
        Y1 = 100;
        X2 = outRC.minX;
        Y2 = 0;
        fDelta_X = nLSpacing;
        fDelta_Y = 0;

        fLimit_X = outRC.maxX;
        fLimit_Y = 1000;
        nLimitType = LIMITTYPE_SMALL2BIG;
        nKMode = KMODE_VERTICAL;
        nSortType = SORTTYPE_Y;
    }
    // 270-315度填充
    else if(fAngle_Hatching < 315)
    {
        k_temp = tan(fAngle_Arc);
        b_temp = outRC.minY - k_temp * outRC.minX;
        Y1 = 5000;
        X1 = (Y1 - b_temp) / k_temp;
        Y2 = 0;
        X2 = (Y2 - b_temp) / k_temp;
        fDelta_X = - nLSpacing / sin(fAngle_Arc);
        fDelta_Y = 0;

        fLimit_X = outRC.maxX - outRC.maxY / k_temp;
        fLimit_Y = 6000;
        nLimitType = LIMITTYPE_SMALL2BIG;
        nKMode = KMODE_NORMAL;
        nSortType = SORTTYPE_Y;
    }
    // 315-360度填充
    else
    {
        k_temp = tan(fAngle_Arc);
        b_temp = outRC.minY - k_temp * outRC.minX;
        X1 = 0;
        Y1 = b_temp;
        X2 = 100;
        Y2 = 100 * k_temp + b_temp;
        fDelta_X = 0;
        fDelta_Y = nLSpacing / cos(fAngle_Arc);

        fLimit_X = 1000;
        fLimit_Y = outRC.maxY - k_temp * outRC.maxX;
        nLimitType = LIMITTYPE_SMALL2BIG;
        nKMode = KMODE_NORMAL;
        nSortType = SORTTYPE_X;
    }

    // 循环生成平行扫描线并计算交点
    uint nAreaCount = curPaths.size();
    uint nPtCount = 0;
    bool bScanDir = false;

    double fDelta_X12 = 0.0, fDelta_Y12 = 0.0;
    double fArea_Cur = 0.0, fArea_Next = 0.0;
    uint nNextPos = 0;

    double fRatio = 0.0;
    double fPt_X = 0.0, fPt_Y = 0.0;
    double fPt_XTemp = 0.0, fPt_YTemp = 0.0;
    bool bPtOnLine = false;

    int nInterPtCnt = 0;

    nTotalLCnt = 0;
    totalHLine.clear();
    qint64 nSerifIndex = -1;

////#pragma omp parallel for
    for(; ;)
    {
//        if(m_bPauseLoop || m_bStopLoop)
//        {
//            goto END_LOOP;
//        }
        // 更新扫描线位置
        X1 += fDelta_X;
        Y1 += fDelta_Y;
        X2 += fDelta_X;
        Y2 += fDelta_Y;
        bScanDir = !bScanDir; // 切换扫描方向,使相邻的填充线方向相反,形成往返扫描模式

        // 计数器初始化
        nInterPtCnt = 0; // 重置交点计数
        nSerifIndex ++; // 填充线序号递增

        // 边界检查
        switch(nLimitType)
        {
        case LIMITTYPE_SMALL2BIG: // 从小到大扫描，如果超出上限边界则结束
            if(X1 >= fLimit_X || X2 >= fLimit_X || Y1 >= fLimit_Y || Y2 >= fLimit_Y)
            {
                return;
            }
            break;
        case LIMITTYPE_BIG2SMALL: // 从大到小扫描，如果超出下限边界则结束
            if(X1 <= fLimit_X || X2 <= fLimit_X || Y1 <= fLimit_Y || Y2 <= fLimit_Y)
            {
                return;
            }
            break;
        }

        fDelta_X12 = X1 - X2; // X方向分量
        fDelta_Y12 = Y1 - Y2; // Y方向分量

        // 计算与轮廓的交点
        QVector<EXPECTPOINTINFO> listExpectPt;

        for(uint iArea = 0; iArea < nAreaCount; iArea ++) // 遍历所有轮廓
        {
            const Path *lpTempP = &curPaths[iArea];

            if(!lineCrossRC(X1, Y1, X2, Y2, listRc.at(int(iArea)))) continue;

            nPtCount = lpTempP->size();
////#pragma omp parallel for
            for(uint iPt = 0; iPt < nPtCount; iPt ++) // 遍历轮廓上的所有点
            {
                bPtOnLine = false;
                // 获取下一个点的索引,如果是最后一个点则回到起点
                nNextPos = iPt + 1;
                nNextPos = nNextPos > nPtCount - 1 ? 0 : nNextPos;

                // 计算当前点与扫描线的面积符号(用于判断点在线的哪一侧)
                fArea_Cur = fDelta_X12 * (lpTempP->at(iPt).Y - Y2) - fDelta_Y12 * (lpTempP->at(iPt).X - X2);
                if(0.0 == fArea_Cur) // 点在线上,跳过
                {
                    continue;
                }

                // 计算下一点与扫描线的面积符号
                fArea_Next = fDelta_X12 * (lpTempP->at(nNextPos).Y - Y2) - fDelta_Y12 * (lpTempP->at(nNextPos).X - X2);

                // 如果下一点在线上,继续寻找直到找到不在线上的点
                while(nNextPos != iPt && 0.0 == fArea_Next)
                {
                    // 记录在线上的点坐标
                    fPt_XTemp = lpTempP->at(nNextPos).X;
                    fPt_YTemp = lpTempP->at(nNextPos).Y;
                    bPtOnLine = true;

                    nNextPos ++;
                    nNextPos = nNextPos > nPtCount - 1 ? 0 : nNextPos;
                    fArea_Next = fDelta_X12 * (lpTempP->at(nNextPos).Y - Y2) - fDelta_Y12 * (lpTempP->at(nNextPos).X - X2);
                }
                // 找了一圈都没找到,跳过
                if(nNextPos == iPt)
                {
                    continue;
                }
                // 如果当前点和下一点在线的两侧(面积符号相反),说明线与轮廓相交
                if(fArea_Cur * fArea_Next < 0)
                {
                    if(bPtOnLine) // 如果有点在线上,直接使用该点作为交点
                    {
                        fPt_X = fPt_XTemp;
                        fPt_Y = fPt_YTemp;
                    }
                    else // 否则根据面积比例计算交点位置
                    {
                        fRatio = fabs(fArea_Cur / fArea_Next);

                        // 根据扫描线类型计算交点坐标
                        switch(nKMode)
                        {
                        case KMODE_HORIZONTAL: // 水平扫描
                            fPt_X = (lpTempP->at(iPt).X + lpTempP->at(nNextPos).X * fRatio) / (1 + fRatio);
                            fPt_Y = Y2;
                            break;
                        case KMODE_NORMAL: // 普通角度扫描
                            fPt_X = (lpTempP->at(iPt).X + lpTempP->at(nNextPos).X * fRatio) / (1 + fRatio);
                            fPt_Y = (fPt_X - X2) * (Y1 - Y2) / (X1 - X2) + Y2;
                            break;
                        case KMODE_VERTICAL: // 垂直扫描
                            fPt_X = X1;
                            fPt_Y = (lpTempP->at(iPt).Y + lpTempP->at(nNextPos).Y * fRatio) / (1 + fRatio);
                            break;
                        }
                    }
                    // 记录交点信息
                    EXPECTPOINTINFO expectPt;
                    expectPt.nAreaIndex = iArea;
                    expectPt.nPtPos = nNextPos;
                    expectPt.X = fPt_X;
                    expectPt.Y = fPt_Y;
                    listExpectPt << std::move(expectPt);
                    nInterPtCnt ++;
//                    qDebug() << X1 << Y1 << X2 << Y2 << iPt << nNextPos << fRatio;
//                    qDebug() << expectPt.nAreaIndex << expectPt.nPtPos << expectPt.X << expectPt.Y;
                }
            }
        }
        // 如果交点数为奇数,说明扫描线与轮廓相交,否则跳过
        if(nInterPtCnt & 0x1)
        {
            qDebug() << "Error Count!!";
        }

        // 当有多于1个交点时,处理填充线
        if(nInterPtCnt > 1)
        {
            HATCHINGLINE listHatchingLine;
            listHatchingLine.nLineIndex = nSerifIndex; // 设置填充线索引

            sortPtOnLine(nSortType, listExpectPt);  // 按指定方式对交点排序
//            qDebug() << listExpectPt;
            int nLineCnt = nInterPtCnt / 2; // 计算填充线段数量(每两个交点形成一条线)
            nTotalLCnt += nLineCnt;
            listHatchingLine.listLineCoor.resize(nLineCnt);
            if(bScanDir)
            {
                // 正向扫描:按顺序配对交点生成填充线
                for(int iLine  = 0; iLine < nLineCnt; iLine ++)
                {
                    int nLIndex = iLine << 1; // 乘2获取交点对的起始索引
                    // 设置线段起点终点坐标和相关信息
                    listHatchingLine.listLineCoor[iLine].X1 = qRound(listExpectPt.at(nLIndex).X);
                    listHatchingLine.listLineCoor[iLine].Y1 = qRound(listExpectPt.at(nLIndex).Y);
                    listHatchingLine.listLineCoor[iLine].X2 = qRound(listExpectPt.at(nLIndex + 1).X);
                    listHatchingLine.listLineCoor[iLine].Y2 = qRound(listExpectPt.at(nLIndex + 1).Y);

                    listHatchingLine.listLineCoor[iLine].nArea_1 = listExpectPt.at(nLIndex).nAreaIndex;
                    listHatchingLine.listLineCoor[iLine].nArea_2 = listExpectPt.at(nLIndex + 1).nAreaIndex;
                    listHatchingLine.listLineCoor[iLine].nPtPos_1 = listExpectPt.at(nLIndex).nPtPos;
                    listHatchingLine.listLineCoor[iLine].nPtPos_2 = listExpectPt.at(nLIndex + 1).nPtPos;
                    // listHatchingLine.listLineCoor << mDrawLine;
                }
            }
            else if(bInner)
            {   
                // 内部填充:交换交点对的顺序生成填充线
                for(int iLine  = 0; iLine < nLineCnt; iLine ++)
                {
                    int nLIndex = iLine << 1;
                    listHatchingLine.listLineCoor[iLine].X1 = qRound(listExpectPt.at(nLIndex + 1).X);
                    listHatchingLine.listLineCoor[iLine].Y1 = qRound(listExpectPt.at(nLIndex + 1).Y);
                    listHatchingLine.listLineCoor[iLine].X2 = qRound(listExpectPt.at(nLIndex).X);
                    listHatchingLine.listLineCoor[iLine].Y2 = qRound(listExpectPt.at(nLIndex).Y);

                    listHatchingLine.listLineCoor[iLine].nArea_1 = listExpectPt.at(nLIndex + 1).nAreaIndex;
                    listHatchingLine.listLineCoor[iLine].nArea_2 = listExpectPt.at(nLIndex).nAreaIndex;
                    listHatchingLine.listLineCoor[iLine].nPtPos_1 = listExpectPt.at(nLIndex + 1).nPtPos;
                    listHatchingLine.listLineCoor[iLine].nPtPos_2 = listExpectPt.at(nLIndex).nPtPos;
                    // listHatchingLine.listLineCoor << mDrawLine;
                }
            }
            else
            {
                // 反向扫描:从后向前配对交点生成填充线
                for(int iLine  = 0; iLine < nLineCnt; iLine ++)
                {
                    int nLIndex = iLine << 1;
                    listHatchingLine.listLineCoor[iLine].X1 = qRound(listExpectPt.at(nInterPtCnt - 1 - nLIndex).X);
                    listHatchingLine.listLineCoor[iLine].Y1 = qRound(listExpectPt.at(nInterPtCnt - 1 - nLIndex).Y);
                    listHatchingLine.listLineCoor[iLine].X2 = qRound(listExpectPt.at(nInterPtCnt - 1 - nLIndex - 1).X);
                    listHatchingLine.listLineCoor[iLine].Y2 = qRound(listExpectPt.at(nInterPtCnt - 1 - nLIndex - 1).Y);

                    listHatchingLine.listLineCoor[iLine].nArea_1 = listExpectPt.at(nInterPtCnt - 1 - nLIndex).nAreaIndex;
                    listHatchingLine.listLineCoor[iLine].nArea_2 = listExpectPt.at(nInterPtCnt - 1 - nLIndex - 1).nAreaIndex;
                    listHatchingLine.listLineCoor[iLine].nPtPos_1 = listExpectPt.at(nInterPtCnt - 1 - nLIndex).nPtPos;
                    listHatchingLine.listLineCoor[iLine].nPtPos_2 = listExpectPt.at(nInterPtCnt - 1 - nLIndex - 1).nPtPos;
                    // listHatchingLine.listLineCoor << mDrawLine;
                }
            }
            // 如果生成了有效的填充线,添加到总的填充线集合中
            if(listHatchingLine.listLineCoor.count()) totalHLine << std::move(listHatchingLine);
        }
    }
}

/// 绘制分支结构的轮廓路径
/// @brief 递归绘制分支结构中的所有节点轮廓路径
/// @param branch 输入分支结构
/// @param listSLines 输出扫描线列表
/// @details 实现步骤:
/// 1. 绘制当前分支的所有节点轮廓
/// 2. 递归处理所有子分支
void AlgorithmHatching::drawBranch(const PocketBranch &branch, QVector<SCANLINE> &listSLines)
{
    // 绘制当前分支的所有节点轮廓
    for(const auto &node : branch.mNodeList)
    {
        // 根据分支类型选择不同的绘制模式(外轮廓=1,内轮廓=2)
        drawPathToScanLine(node.circle, listSLines, (PocketBranch::Outer == branch.mCircleType) ? 1 : 2);
    }

    // 递归处理所有子分支
    for(const auto &subBranch : branch.mSubBranchList)
    {
        drawBranch(subBranch, listSLines);
    }
}

/// 绘制分支结构的螺旋路径
/// @brief 递归绘制分支结构中的所有螺旋填充路径
/// @param branch 输入分支结构
/// @param listSLines 输出扫描线列表
/// @details 实现步骤:
/// 1. 绘制当前分支的螺旋路径
/// 2. 递归处理所有子分支
void AlgorithmHatching::drawBranch_spiral(const PocketBranch &branch, QVector<SCANLINE> &listSLines)
{
#ifdef USE_PROCESSOR_EXTEND
    // 对螺旋路径进行偏移处理
    drawPathOffset(branch.spiralPath, IntPoint(25000, 0), 0);
#endif

    // 将螺旋路径转换为扫描线
    drawSpiralPathToScanLine(branch.spiralPath, listSLines);

    // 递归处理所有子分支
    for(const auto &subBranch : branch.mSubBranchList)
    {
        drawBranch_spiral(subBranch, listSLines);
    }
}

/// 绘制分支结构的费马螺旋路径
/// @brief 递归绘制分支结构中的所有费马螺旋填充路径(当前已注释掉)
/// @param branch 输入分支结构
/// @param listSLines 输出扫描线列表
/// @details 实现步骤:
/// 1. 对费马螺旋路径进行偏移处理
/// 2. 将螺旋路径转换为扫描线
/// 3. 递归处理所有子分支
void AlgorithmHatching::drawBranch_fermatSpiral(const PocketBranch &branch, QVector<SCANLINE> &listSLines)
{
// 注释掉的功能:
// 1. 对费马螺旋路径进行(25000,25000)的偏移
// 2. 将费马螺旋路径转换为扫描线
// 3. 递归处理子分支的费马螺旋路径
//#ifdef USE_PROCESSOR_EXTEND
//    drawPathOffset(branch.fermatSpiralPath, IntPoint(25000, 25000), 2);
//#endif

//    drawSpiralPathToScanLine(branch.fermatSpiralPath, listSLines);
//    for(const auto &subBranch : branch.mSubBranchList)
//    {
//        drawBranch_fermatSpiral(subBranch, listSLines);
//    }
}


/// 计算环形填充路径
/// @brief 生成轮廓的环形偏移填充路径
/// @param curPaths 输入轮廓路径集合
/// @param listSLines 输出扫描线列表
/// @param nSpacing 填充间距
/// @details 实现步骤:
/// 1. 构建轮廓的树形结构
/// 2. 遍历树结构绘制环形填充路径
void AlgorithmHatching::calcRingHatching(const Paths &curPaths, QVector<SCANLINE> &listSLines, const int &nSpacing)
{
    // 构建轮廓的树形结构
    PocketTree outerTree;
    calcRingTree(curPaths, outerTree, nSpacing);

    // 遍历树结构绘制环形填充路径
    for(const auto &branch : qAsConst(outerTree.branchList))
    {
        drawBranch(branch, listSLines);
    }
}

/// 计算螺旋填充路径
/// @brief 生成轮廓的螺旋填充路径
/// @param curPaths 输入轮廓路径集合
/// @param listSLines 输出扫描线列表
/// @param nSpacing 填充间距
/// @details 实现步骤:
/// 1. 构建轮廓的树形结构
/// 2. 生成螺旋填充路径
/// 3. 遍历树结构绘制螺旋路径
void AlgorithmHatching::calcSpiralHatching(const Paths &curPaths, QVector<SCANLINE> &listSLines, const int &nSpacing)
{
    // 构建轮廓的树形结构
    PocketTree outerTree;
    calcRingTree(curPaths, outerTree, nSpacing);

    // 生成螺旋填充路径
    createSpiral(outerTree, nSpacing * nSpacing);

    // 遍历树结构绘制螺旋路径
    for(const auto &branch : outerTree.branchList)
    {
        drawBranch_spiral(branch, listSLines);
    }
}

/// 计算费马螺旋填充路径
/// @brief 生成轮廓的费马螺旋填充路径
/// @param curPaths 输入轮廓路径集合
/// @param listSLines 输出扫描线列表
/// @param nSpacing 填充间距
/// @details 实现步骤:
/// 1. 构建轮廓的树形结构
/// 2. 生成费马螺旋填充路径
/// 3. 遍历树结构绘制费马螺旋路径
void AlgorithmHatching::calcFermatSpiralHatching(const Paths &curPaths, QVector<SCANLINE> &listSLines, const int &nSpacing)
{
    // 构建轮廓的树形结构
    PocketTree outerTree;
    calcRingTree(curPaths, outerTree, nSpacing);

    // 生成费马螺旋填充路径
    createFermatSpiral(outerTree, nSpacing * nSpacing);

    // 遍历树结构绘制费马螺旋路径
    for(const auto &branch : outerTree.branchList)
    {
        drawBranch_fermatSpiral(branch, listSLines);
    }
}


/// 计算轮廓的环形树结构
/// @brief 通过递归偏移生成轮廓的嵌套环形树结构
/// @param curPaths 输入轮廓路径集合
/// @param outerTree 输出外部轮廓树结构
/// @param nSpacing 偏移间距
/// @details 实现步骤:
/// 1. 初始化内外轮廓树结构
/// 2. 对轮廓进行递归偏移,生成嵌套环
/// 3. 更新并排序树结构
void AlgorithmHatching::calcRingTree(const Paths &curPaths, PocketTree &outerTree, const int &nSpacing)
{
    // 创建内部轮廓树结构
    InnerPocketTree innerTree;

    // 复制输入路径
    Paths tempPath = curPaths;
    Paths target, totalPath;

#ifdef USE_PROCESSOR_EXTEND
    // 对路径进行显示偏移(用于调试)
    drawPathOffset(tempPath, IntPoint(0, 25000));
#endif

    // 创建初始的轮廓树结构
    createPocketTree(tempPath, outerTree, innerTree);

    // 递归偏移生成嵌套环
    int nLevel = 0;
    while(tempPath.size())
    {
        // 对当前轮廓进行偏移
        getOffsetPaths(&tempPath, &target, nSpacing, 0, 0);

#ifdef USE_PROCESSOR_EXTEND
        // 对偏移结果进行显示(用于调试)
        drawPathOffset(target, IntPoint(0, 25000));
#endif

        // 更新树结构,记录当前层级
        updatePocketTree(target, outerTree, innerTree, ++ nLevel);
        tempPath = target;
    }

    // 对树结构进行排序优化
    sortPocketTree(outerTree, innerTree);
}

/// 计算扫描线路径
/// @brief 根据不同模式生成填充线的扫描路径
/// @param nMode 填充模式(原始/最近路径/三角形/贪婪)
/// @param listHatchingLine 填充线列表
/// @param listSLines 输出扫描线列表
/// @param nTotalLCnt 总线段数
/// @param nLSConnecter 连接器参数
/// @param nCLineFactor 线段因子
/// @details 实现步骤:
/// 1. 根据模式选择不同的路径生成策略
/// 2. 遍历填充线生成扫描路径
/// 3. 优化连接顺序减少空走
void AlgorithmHatching::calcSPLine(const int &nMode, TOTALHATCHINGLINE &listHatchingLine, QVector<SCANLINE> &listSLines,
                                   const int &nTotalLCnt, const int &nLSConnecter, const int &nCLineFactor)
{
    if(LINETYPE_ORIGINAL == nMode)
    {
        // 原始模式:直接按顺序输出填充线
        int X1 = 0, Y1 = 0, X2 = 0, Y2 = 0;
        int nLineCnt_Hor = listHatchingLine.count();
        int nLineCnt_PerLine = 0;
        // 遍历所有填充线
        for(int iLineIndex_Hor = 0; iLineIndex_Hor < nLineCnt_Hor; iLineIndex_Hor ++)
        {
            nLineCnt_PerLine = listHatchingLine.at(iLineIndex_Hor).listLineCoor.count();
            for(int iLineIndex_PerLine = 0; iLineIndex_PerLine < nLineCnt_PerLine; iLineIndex_PerLine ++)
            {
                // 获取线段端点坐标并生成扫描线
                X1 = listHatchingLine.at(iLineIndex_Hor).listLineCoor.at(iLineIndex_PerLine).X1;
                Y1 = listHatchingLine.at(iLineIndex_Hor).listLineCoor.at(iLineIndex_PerLine).Y1;
                X2 = listHatchingLine.at(iLineIndex_Hor).listLineCoor.at(iLineIndex_PerLine).X2;
                Y2 = listHatchingLine.at(iLineIndex_Hor).listLineCoor.at(iLineIndex_PerLine).Y2;
                //emit datas_AddLine_2D(X1, Y1, X2, Y2, m_fCurHei, UNITFACTOR);
                scanner_plotLine(X1, Y1, X2, Y2, listSLines);
            }
        }
        listHatchingLine.clear();
    }
    else if(LINETYPE_SORTPATH_NEAR == nMode)
    {
        // 最近路径模式:选择最近的下一条线
        // 实现最近邻搜索算法优化路径
        // 初始化坐标和计数器
        int X1 = 0, Y1 = 0, X2 = 0, Y2 = 0;  // 线段端点坐标
        qint64 nTempLineCount = 0;  // 已处理线段计数
        int nNextIndex_L = 0;  // 下一条线的层索引
        int nNextIndex_P = 0;  // 下一条线的点索引
        bool bFirstLine = true;  // 是否第一条线标志

        // 处理所有填充线直到完成
        while(nTempLineCount < nTotalLCnt) {
            // 获取第一条线段
            if(getFirstLine(X2, Y2, &nNextIndex_L, &nNextIndex_P, listHatchingLine, bFirstLine)) {
                // 获取线段端点坐标
                X1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).X1;
                Y1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).Y1;
                X2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).X2;
                Y2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).Y2;
                
                // 移除已处理的线段并生成扫描线
                listHatchingLine[nNextIndex_L].listLineCoor.removeAt(nNextIndex_P);
                scanner_plotLine(X1, Y1, X2, Y2, listSLines);
                nTempLineCount++;

                // 循环获取最近的下一条线段
                while(getNextLine(X2, Y2, &nNextIndex_L, &nNextIndex_P, listHatchingLine, nLSConnecter)) {
                    // 获取下一条线段坐标
                    X1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).X1;
                    Y1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).Y1;
                    X2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).X2;
                    Y2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).Y2;

                    // 移除已处理的线段并生成扫描线
                    listHatchingLine[nNextIndex_L].listLineCoor.removeAt(nNextIndex_P);
                    scanner_plotLine(X1, Y1, X2, Y2, listSLines);
                    nTempLineCount++;
                }
            }
        }
    }
    else if(LINETYPE_SORTPATH_TRIANGLE == nMode)
    {
        // 三角形模式:考虑线段长度和角度关系优化路径
        // 实现基于三角形关系的路径优化
        // 初始化坐标和计数器
        uint nPtPos_S1 = 0;
        uint nPtPos_S2 = 0;
        uint nAreaIndex_S1 = 0;
        uint nAreaIndex_S2 = 0;
        int nLength_S = 0;

        int X1_O = 0, Y1_O = 0, X2_O = 0, Y2_O = 0;
        int X1 = 0, Y1 = 0, X2 = 0, Y2 = 0;
        uint nAreaIndex_1 = 0, nAreaIndex_2 = 0;
        uint nPtPos_1 = 0, nPtPos_2 = 0;
        int nLength = 0;
        qint64 nTempLineCount = 0;
        int nNextIndex_L = 0;
        int nNextIndex_P = 0;
        bool bJump = false;
        bool bConnect = false;
        int nDX = 0, nDY = 0;
        bool bFirstLine = true;
        
        // 处理所有填充线直到完成
        while(nTempLineCount < nTotalLCnt)
        {
            // 根据线段因子选择不同的查找方式
            bool bFindLine = nCLineFactor < 11
                ? getFirstLine_Greed(X2_O, Y2_O, &nNextIndex_L, &nNextIndex_P, listHatchingLine, bFirstLine) :
                getFirstLine(X2_O, Y2_O, &nNextIndex_L, &nNextIndex_P, listHatchingLine, bFirstLine);
            if(bFindLine)
            {
                // 获取并处理当前线段信息
                X1_O = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).X1;
                Y1_O = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).Y1;
                X2_O = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).X2;
                Y2_O = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).Y2;
                // 计算线段长度
                nDX = abs(X1_O - X2_O);
                nDY = abs(Y1_O - Y2_O);
                nLength_S = int(sqrt(pow(nDX, 2) + pow(nDY, 2)));//nDX + nDY;
                // 获取线段的区域和点索引信息
                nAreaIndex_S1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).nArea_1;
                nAreaIndex_S2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).nArea_2;
                nPtPos_S1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).nPtPos_1;
                nPtPos_S2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).nPtPos_2;
                // 移除已处理的线段并生成扫描线
                listHatchingLine[nNextIndex_L].listLineCoor.removeAt(nNextIndex_P);
//                if(0 == m_TotalLines.at(nNextIndex_L).count())
//                {
//                    m_TotalLines.removeAt(nNextIndex_L);
//                    m_TotalLineIndex.removeAt(nNextIndex_L);
//                }
                // 重置跳转标志
                bJump = false;

                // 生成扫描线并更新计数
                scanner_plotLine(X2_O, Y2_O, X1_O, Y1_O, listSLines);
                nTempLineCount ++;
                // 查找并处理下一条线段
                while(getNextLine(X2_O, Y2_O, &nNextIndex_L, &nNextIndex_P, listHatchingLine, nLSConnecter))
                {
                    // 获取下一线段的端点坐标和区域信息
                    X1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).X1;
                    Y1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).Y1;
                    X2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).X2;
                    Y2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).Y2;
                    
                    // 获取区域和点索引信息
                    nAreaIndex_1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).nArea_1;
                    nAreaIndex_2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).nArea_2;
                    nPtPos_1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).nPtPos_1;
                    nPtPos_2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).nPtPos_2;
                    // 移除已处理的线段
                    listHatchingLine[nNextIndex_L].listLineCoor.removeAt(nNextIndex_P);
//                    if(0 == m_TotalLines.at(nNextIndex_L).count())
//                    {
//                        m_TotalLines.removeAt(nNextIndex_L);
//                        m_TotalLineIndex.removeAt(nNextIndex_L);
//                    }

                    // 判断是否可以连接
                    bConnect = false;
                    // 检查区域匹配条件
                    if(nAreaIndex_S1 == nAreaIndex_2 && nAreaIndex_S2 == nAreaIndex_1)
                    {
                        // 检查点位置关系
                        if((nAreaIndex_S1 == nAreaIndex_S2) ?
                                ((fabs(double(nPtPos_1) - nPtPos_S2) < fabs(double(nPtPos_1) - nPtPos_S1))
                                 &&(fabs(double(nPtPos_2) - nPtPos_S1) < fabs(double(nPtPos_2) - nPtPos_S2))
                                 ? true : false ) : true)
                        {
                            // 计算线段长度并检查长度比例
                            nDX = abs(X1 - X2);
                            nDY = abs(Y1 - Y2);
                            nLength = int(sqrt(pow(nDX, 2) + pow(nDY, 2)));//nDX + nDY;
                            if(double(qMax(nLength_S, nLength)) / double(qMin(nLength_S, nLength)) < 2)
                            {
                                //emit datas_AddLine_2D(X1_O, Y1_O, X1, Y1, m_fCurHei, UNITFACTOR);
                                // 可以连接,生成连接扫描线
                                scanner_outLine(X1, Y1, listSLines);
                                bConnect = true;
                                bJump = true;
                            }
                        }
                    }
                    // 根据是否连接选择不同的扫描方式
                    if(false  == bConnect)
                    {
                        if(bJump)
                        {
                            //emit datas_AddLine_2D(X1_O, Y1_O, X2_O, Y2_O, m_fCurHei, UNITFACTOR);
                            scanner_outLine(X2_O, Y2_O, listSLines);
                        }
                        //emit datas_AddLine_2D(X2, Y2, X1, Y1, m_fCurHei, UNITFACTOR);
                        scanner_plotLine(X2, Y2, X1, Y1, listSLines);
                        bJump = false;
                    }
                    // 更新计数和存储当前线段信息
                    nTempLineCount ++;

                    // X1_O = X1;
                    // Y1_O = Y1;
                    X2_O = X2;
                    Y2_O = Y2;
                    nLength_S = nLength;
                    nAreaIndex_S1 = nAreaIndex_1;
                    nAreaIndex_S2 = nAreaIndex_2;
                    nPtPos_S1 = nPtPos_1;
                    nPtPos_S2 = nPtPos_2;
                }
                if(bJump)
                {
                    //emit datas_AddLine_2D(X1_O, Y1_O, X2_O, Y2_O, m_fCurHei, UNITFACTOR);
                    scanner_outLine(X2_O, Y2_O, listSLines);
                }
            }
        }
    }
    else if(LINETYPE_SORTPATH_GREED == nMode)
    {
        // 贪婪模式:在局部范围内寻找最优下一条线
        // 实现贪婪算法优化路径
        int nLineCnt = nTotalLCnt; // 总线段数
        int X1 = 0, Y1 = 0, X2 = 0, Y2 = 0; // 当前线段的端点坐标
        int nNextIndex_L = 0; // 下一线段索引
        int nNextIndex_P = 0; // 下一线段端点索引
        bool bFirstLine = true; // 是否为第一条线段

        double fMinDis = 0; // 最小距离
        double fTempDis = 0; // 临时距离
        int nCurLine = 0; // 当前线段索引
        int nCurPt = 0; // 当前线段端点索引
        bool bFindLine = false; // 是否找到下一线段

        int nStep = 1; // 步长
        // 处理所有填充线
        while(nLineCnt > 0)
        {
            // 获取第一条线段
            if(getFirstLine_Greed(X2, Y2, &nNextIndex_L, &nNextIndex_P, listHatchingLine, bFirstLine))
            {
                // 获取当前线段端点坐标
                X1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).X1;
                Y1 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).Y1;
                X2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).X2;
                Y2 = listHatchingLine.at(nNextIndex_L).listLineCoor.at(nNextIndex_P).Y2;
                listHatchingLine[nNextIndex_L].listLineCoor.removeAt(nNextIndex_P);
                -- nLineCnt;
                scanner_plotLine(X1, Y1, X2, Y2, listSLines);

                // 局部搜索最优下一线段
                nStep = 1;
                do
                {
                    bFindLine = false;
                    fMinDis = 1E20; // 初始化最小距离为一个很大的值
                    
                    // 向前搜索
                    int nTempLine = nNextIndex_L - nStep;
                    if(nTempLine > -1)
                    {
                        // 计算当前行所有线段到目标点的距离
                        QVector<LINECOOR> *listLine = &listHatchingLine[nTempLine].listLineCoor;
                        int nSegmentCnt = listLine->count();
                        for(int iSeq = 0; iSeq < nSegmentCnt; ++ iSeq)
                        {
                            // 计算欧氏距离
                            fTempDis = sqrt(pow(listLine->at(iSeq).X1 - X2, 2) +
                                            pow(listLine->at(iSeq).Y1 - Y2, 2));
                            // 更新最小距离
                            if(fTempDis < fMinDis)
                            {
                                fMinDis = fTempDis;
                                nCurLine = nTempLine;
                                nCurPt = iSeq;
                                bFindLine = true;
                            }
                        }
                    }
                    // 向后搜索
                    nTempLine = nNextIndex_L + nStep;
                    if(nTempLine < listHatchingLine.count())
                    {
                        QVector<LINECOOR> *listLine = &listHatchingLine[nTempLine].listLineCoor;
                        int nSegmentCnt = listLine->count();
                        for(int iSeq = 0; iSeq < nSegmentCnt; ++ iSeq)
                        {
                            fTempDis = sqrt(pow(listLine->at(iSeq).X1 - X2, 2) +
                                            pow(listLine->at(iSeq).Y1 - Y2, 2));
                            if(fTempDis < fMinDis)
                            {
                                fMinDis = fTempDis;
                                nCurLine = nTempLine;
                                nCurPt = iSeq;
                                bFindLine = true;
                            }
                        }
                    }
                    // 搜索当前行(仅在步长为1时)
                    if(1 == nStep)
                    {
                        nTempLine = nNextIndex_L;
                        QVector<LINECOOR> *listLine = &listHatchingLine[nTempLine].listLineCoor;
                        int nSegmentCnt = listLine->count();
                        for(int iSeq = 0; iSeq < nSegmentCnt; ++ iSeq)
                        {
                            fTempDis = sqrt(pow(listLine->at(iSeq).X1 - X2, 2) +
                                            pow(listLine->at(iSeq).Y1 - Y2, 2));
                            if(fTempDis < fMinDis)
                            {
                                fMinDis = fTempDis;
                                nCurLine = nTempLine;
                                nCurPt = iSeq;
                                bFindLine = true;
                            }
                        }
                    }
                    // 如果找到合适的线段，处理它
                    if(bFindLine)
                    {
                        X1 = listHatchingLine.at(nCurLine).listLineCoor.at(nCurPt).X1;
                        Y1 = listHatchingLine.at(nCurLine).listLineCoor.at(nCurPt).Y1;
                        X2 = listHatchingLine.at(nCurLine).listLineCoor.at(nCurPt).X2;
                        Y2 = listHatchingLine.at(nCurLine).listLineCoor.at(nCurPt).Y2;
                        listHatchingLine[nCurLine].listLineCoor.removeAt(nCurPt);
                        -- nLineCnt;
                        scanner_plotLine(X1, Y1, X2, Y2, listSLines);
                    }

                    ++ nStep; // 增加搜索范围
                }
                while(bFindLine/*(bFindLine || nStep < 20)*/ && nLineCnt > 0); // 继续搜索直到找不到合适的线段
            }
        }
    }
}

#ifdef USE_PROCESSOR_EXTEND
#include <QTime>
void AlgorithmHatching::drawPathOffset(const Path &path, const IntPoint &offsetCoor, const ulong &ms)
{
    uint nPtCnt = path.size();
    if(nPtCnt < 2) return;

    qint64 nLastX = path[0].X, nLastY = path[0].Y;
    uint iPt = 1;
    srand(uint(QTime::currentTime().msec()));

    quint8 base = rand() % 200;
    quint8 r = base + quint8(rand() % (256 - base));

    quint8 g = base + quint8(rand() % (256 - base));

    base = 255 - base;
    quint8 b = base + quint8(rand() % (256 - base));
    do
    {
        glProcessor->drawLines(nLastX + offsetCoor.X, nLastY + offsetCoor.Y,
                               path[iPt].X + offsetCoor.X, path[iPt].Y + offsetCoor.Y, r, g, b);
        nLastX = path[iPt].X;
        nLastY = path[iPt].Y;
        QThread::msleep(ms);
    }
    while(++ iPt < nPtCnt);
}

void AlgorithmHatching::drawPathOffset(const Paths &totalPath, const IntPoint &offsetCoor, const ulong &ms)
{

    int nDrawType = 1;
    for(const auto &path : totalPath)
    {
        uint nPtCnt = path.size();
        if(nPtCnt < 2) continue;

        nDrawType = Area(path) >= 0 ? 1 : 2;
        qint64 nLastX = path[0].X, nLastY = path[0].Y;
        uint iPt = 1;
        do
        {
            glProcessor->drawLines(nLastX + offsetCoor.X, nLastY + offsetCoor.Y,
                                   path[iPt].X + offsetCoor.X, path[iPt].Y + offsetCoor.Y, nDrawType);
            nLastX = path[iPt].X;
            nLastY = path[iPt].Y;
        }
        while(++ iPt < nPtCnt);
        glProcessor->drawLines(nLastX + offsetCoor.X, nLastY + offsetCoor.Y,
                               path[0].X + offsetCoor.X, path[0].Y + offsetCoor.Y, nDrawType);
        if(ms) QThread::msleep(ms);
    }
}
#endif

///
/// @brief 将路径转换为扫描线序列
/// @param totalPath 输入路径集合
/// @param listSLines 输出扫描线序列
/// @details 实现步骤:
/// 1. 遍历每条路径
/// 2. 生成起点跳转指令
/// 3. 生成连续的扫描线指令
/// 4. 闭合回起点
///
void AlgorithmHatching::drawPathToScanLine(const Paths &totalPath, QVector<SCANLINE> &listSLines)
{
    // 遍历所有路径
    for(const auto &path : totalPath)
    {
        uint nPtCnt = path.size();
        if(nPtCnt < 2) continue;

        // 生成起点跳转指令
        scanner_jumpPos(path[0].X, path[0].Y, listSLines);

#ifdef USE_PROCESSOR_EXTEND
        qint64 nLastX = path[0].X, nLastY = path[0].Y;
#endif
        
        // 生成连续的扫描线指令
        uint iPt = 1;
        do
        {
            scanner_outLine(path[iPt].X, path[iPt].Y, listSLines);

#ifdef USE_PROCESSOR_EXTEND
            glProcessor->drawLines(nLastX, nLastY, path[iPt].X, path[iPt].Y, 1);
            nLastX = path[iPt].X;
            nLastY = path[iPt].Y;
#endif
        }
        while(++ iPt < nPtCnt);

        // 闭合回起点
        scanner_outLine(path[0].X, path[0].Y, listSLines);

#ifdef USE_PROCESSOR_EXTEND
        glProcessor->drawLines(nLastX, nLastY, path[0].X, path[0].Y, 1);
#endif
    }
}

///
/// @brief 将单条路径转换为扫描线序列,支持坐标偏移
/// @param path 输入单条路径
/// @param listSLines 输出扫描线序列
/// @param type 路径类型
/// @param offsetCoor 坐标偏移量
/// @details 实现步骤:
/// 1. 生成起点跳转指令
/// 2. 生成连续的扫描线指令,支持坐标偏移
/// 3. 闭合回起点
///
void AlgorithmHatching::drawPathToScanLine(const Path &path, QVector<SCANLINE> &listSLines, const int &type, const IntPoint &offsetCoor)
{
    uint nPtCnt = path.size();
    if(nPtCnt < 2) return;

    // 生成起点跳转指令
    scanner_jumpPos(path[0].X, path[0].Y, listSLines);

#ifdef USE_PROCESSOR_EXTEND
    qint64 nLastX = path[0].X, nLastY = path[0].Y;
#endif

    // 生成连续的扫描线指令
    uint iPt = 1;
    do
    {
        scanner_outLine(path[iPt].X, path[iPt].Y, listSLines);

#ifdef USE_PROCESSOR_EXTEND
        // 应用坐标偏移进行图形显示
        glProcessor->drawLines(nLastX + offsetCoor.X, nLastY + offsetCoor.Y,
                             path[iPt].X + offsetCoor.X, path[iPt].Y + offsetCoor.Y, type);
        nLastX = path[iPt].X;
        nLastY = path[iPt].Y;
#endif
    }
    while(++ iPt < nPtCnt);

    // 闭合回起点
    scanner_outLine(path[0].X, path[0].Y, listSLines);

#ifdef USE_PROCESSOR_EXTEND
    // 绘制闭合线段并延时显示
    glProcessor->drawLines(nLastX + offsetCoor.X, nLastY + offsetCoor.Y,
                         path[0].X + offsetCoor.X, path[0].Y + offsetCoor.Y, type);
    QThread::msleep(50);
#endif
}


///
/// @brief 将螺旋路径转换为扫描线序列
/// @param path 输入螺旋路径
/// @param listSLines 输出扫描线序列
/// @details 实现步骤:
/// 1. 生成起点跳转指令
/// 2. 生成连续的扫描线指令
/// 3. 螺旋路径无需闭合回起点
///
void AlgorithmHatching::drawSpiralPathToScanLine(const Path &path, QVector<SCANLINE> &listSLines)
{
    uint nPtCnt = path.size();
    if(nPtCnt < 2) return;

    // 生成起点跳转指令
    scanner_jumpPos(path[0].X, path[0].Y, listSLines);

    // 生成连续的扫描线指令
    uint iPt = 1;
    do
    {
        scanner_outLine(path[iPt].X, path[iPt].Y, listSLines);
    }
    while(++ iPt < nPtCnt);
}

///
/// @brief 获取下一条要处理的填充线
/// @param nX 当前X坐标
/// @param nY 当前Y坐标
/// @param nLine 输出行索引
/// @param nIndex 输出线段索引
/// @param lpListHLine 填充线列表
/// @param bFirstLine 是否第一条线标志
/// @return 是否找到下一条线
/// @details 实现步骤:
/// 1. 第一条线时直接返回第一个非空行的第一条线
/// 2. 非第一条线时在相邻行中寻找最近的线段:
///    - 搜索下一行
///    - 搜索上一行
///    - 搜索当前行
/// 3. 如果相邻行都没找到,则搜索整个列表
///
bool AlgorithmHatching::getFirstLine(const int &nX, const int &nY, int *nLine, int *nIndex, TOTALHATCHINGLINE &lpListHLine, bool &bFirstLine)
{
    // 处理第一条线的情况
    if(bFirstLine)
    {
        bFirstLine = false;
        int nLineCnt = lpListHLine.count();
        // 返回第一个非空行的第一条线
        for(int iLIndex = 0; iLIndex < nLineCnt; iLIndex ++)
        {
            if(lpListHLine.at(iLIndex).listLineCoor.count() > 0)
            {
                *nIndex = 0;
                *nLine = iLIndex;
                return true;
            }
        }
    }
    else
    {
        *nIndex = -1;
        int nDelta = 0x7FFFFFFF; // 初始化最小距离
        int nLCnt = lpListHLine.count();
        int nLineTemp = 0;

        // 搜索下一行
        int nLIndex_1 = *nLine + 1;
        if(nLIndex_1 < nLCnt)
        {
            int nPointCnt = lpListHLine.at(nLIndex_1).listLineCoor.count();
            if(nPointCnt > 0)
            {
                // 计算到下一行所有线段的距离
                int *nTempDis = new int[uint(nPointCnt)];
                int nDX = 0, nDY = 0;
//#pragma omp parallel for
                for(int iPIndex = 0; iPIndex < nPointCnt; iPIndex ++)
                {
                    nDX = abs(lpListHLine.at(nLIndex_1).listLineCoor.at(iPIndex).X1 - nX);
                    nDY = abs(lpListHLine.at(nLIndex_1).listLineCoor.at(iPIndex).Y1 - nY);
                    nTempDis[iPIndex] = int(sqrt(pow(nDX, 2) + pow(nDY, 2)));//nDX + nDY;
                }
                // 更新最小距离
                for(int iPIndex = 0; iPIndex < nPointCnt; iPIndex ++)
                {
                    if(nTempDis[iPIndex] < nDelta)
                    {
                        nDelta = nTempDis[iPIndex];
                        nLineTemp = nLIndex_1;
                        *nIndex = iPIndex;
                    }
                }
                delete []nTempDis;
            }
        }

        // 搜索上一行(逻辑同上)
        int nLIndex_2 = *nLine - 1;
        if(nLIndex_2 > -1)
        {
            int nPointCnt = lpListHLine.at(nLIndex_2).listLineCoor.count();
            if(nPointCnt > 0)
            {
                int *nTempDis = new int[uint(nPointCnt)];
                int nDX = 0, nDY = 0;
//#pragma omp parallel for
                for(int iPIndex = 0; iPIndex < nPointCnt; iPIndex ++)
                {
                    nDX = abs(lpListHLine.at(nLIndex_2).listLineCoor.at(iPIndex).X1 - nX);
                    nDY = abs(lpListHLine.at(nLIndex_2).listLineCoor.at(iPIndex).Y1 - nY);
                    nTempDis[iPIndex] = int(sqrt(pow(nDX, 2) + pow(nDY, 2)));//nDX + nDY;
                }
                for(int iPIndex = 0; iPIndex < nPointCnt; iPIndex ++)
                {
                    if(nTempDis[iPIndex] < nDelta)
                    {
                        nDelta = nTempDis[iPIndex];
                        nLineTemp = nLIndex_2;
                        *nIndex = iPIndex;
                    }
                }
                delete []nTempDis;
            }
        }

        // 搜索当前行(逻辑同上)
        if(*nLine < nLCnt)
        {
            int nPointCnt = lpListHLine.at(*nLine).listLineCoor.count();
            if(nPointCnt > 0)
            {
                int *nTempDis = new int[uint(nPointCnt)];
                int nDX = 0, nDY = 0;
//#pragma omp parallel for
                for(int iPIndex = 0; iPIndex < nPointCnt; iPIndex ++)
                {
                    nDX = abs(lpListHLine.at(*nLine).listLineCoor.at(iPIndex).X1 - nX);
                    nDY = abs(lpListHLine.at(*nLine).listLineCoor.at(iPIndex).Y1 - nY);
                    nTempDis[iPIndex] = int(sqrt(pow(nDX, 2) + pow(nDY, 2)));//nDX + nDY;
                }
                for(int iPIndex = 0; iPIndex < nPointCnt; iPIndex ++)
                {
                    if(nTempDis[iPIndex] < nDelta)
                    {
                        nDelta = nTempDis[iPIndex];
                        nLineTemp = *nLine;
                        *nIndex = iPIndex;
                    }
                }
                delete []nTempDis;
            }
        }
        // 如果找到最近的线段,返回其位置
        if(-1 != *nIndex)
        {
            *nLine = nLineTemp;
            return true;
        }

        // 如果相邻行都没找到,搜索整个列表
        for(int iLIndex = 0; iLIndex < nLCnt; iLIndex ++)
        {
            if(lpListHLine.at(iLIndex).listLineCoor.count() > 0)
            {
                *nIndex = 0;
                *nLine = iLIndex;
                return true;
            }
        }
    }
    return false;
}

///
/// @brief 获取下一条连接线段
/// @param nX 当前X坐标
/// @param nY 当前Y坐标
/// @param nLine 输出行索引
/// @param nIndex 输出线段索引
/// @param lpListHLine 填充线列表
/// @param nLConneter 连接距离阈值
/// @return 是否找到合适的下一条线
/// @details 实现步骤:
/// 1. 在相邻行中搜索满足条件的线段:
///    - 行索引差值为1
///    - 距离小于连接阈值
///    - 距离是当前最小值
/// 2. 搜索顺序:
///    - 先搜索下一行
///    - 再搜索上一行
///
bool AlgorithmHatching::getNextLine(const int &nX, const int &nY, int *nLine, int *nIndex, TOTALHATCHINGLINE &lpListHLine, const int &nLConneter)
{
    *nIndex = -1;
    int nDelta = 0x7FFFFFFF;    // 初始化最小距离
    int nLCnt = lpListHLine.count();
    int nLineTemp = *nLine;

    // 搜索下一行
    int nLIndex_1 = nLineTemp + 1;
    // 检查是否为相邻行
    if(nLIndex_1 < nLCnt && 1 == abs(lpListHLine.at(nLIndex_1).nLineIndex - lpListHLine.at(nLineTemp).nLineIndex))
    {
        int nPointCnt = lpListHLine.at(nLIndex_1).listLineCoor.count();
        if(nPointCnt > 0)
        {
            // 计算到下一行所有线段的距离
            int *nTempDis = new int[uint(nPointCnt)];
            int nDX = 0, nDY = 0;
//#pragma omp parallel for
            for(int iPIndex = 0; iPIndex < nPointCnt; iPIndex ++)
            {
                nDX = abs(lpListHLine.at(nLIndex_1).listLineCoor.at(iPIndex).X1 - nX);
                nDY = abs(lpListHLine.at(nLIndex_1).listLineCoor.at(iPIndex).Y1 - nY);
                nTempDis[iPIndex] = int(sqrt(pow(nDX, 2) + pow(nDY, 2)));//nDX + nDY;
            }
            // 更新满足连接条件的最小距离
            for(int iPIndex = 0; iPIndex < nPointCnt; iPIndex ++)
            {
                if(nTempDis[iPIndex] < nLConneter && nTempDis[iPIndex] < nDelta)
                {
                    nDelta = nTempDis[iPIndex];
                    nLineTemp = nLIndex_1;
                    *nIndex = iPIndex;
                }
            }
            delete []nTempDis;
        }
    }
    // 搜索上一行(逻辑同上)
    int nLIndex_2 = nLineTemp - 1;
    if(nLIndex_2 > -1 && 1 == abs(lpListHLine.at(nLIndex_2).nLineIndex - lpListHLine.at(nLineTemp).nLineIndex))
    {
        int nPointCnt = lpListHLine.at(nLIndex_2).listLineCoor.count();
        if(nPointCnt > 0)
        {
            int *nTempDis = new int[uint(nPointCnt)];
            int nDX = 0, nDY = 0;
//#pragma omp parallel for
            for(int iPIndex = 0; iPIndex < nPointCnt; iPIndex ++)
            {
                nDX = abs(lpListHLine.at(nLIndex_2).listLineCoor.at(iPIndex).X1 - nX);
                nDY = abs(lpListHLine.at(nLIndex_2).listLineCoor.at(iPIndex).Y1 - nY);
                nTempDis[iPIndex] = int(sqrt(pow(nDX, 2) + pow(nDY, 2)));//nDX + nDY;
            }
            for(int iPIndex = 0; iPIndex < nPointCnt; iPIndex ++)
            {
                if(nTempDis[iPIndex] < nLConneter && nTempDis[iPIndex] < nDelta)
                {
                    nDelta = nTempDis[iPIndex];
                    nLineTemp = nLIndex_2;
                    *nIndex = iPIndex;
                }
            }
            delete []nTempDis;
        }
    }
    // 如果找到合适的线段,返回其位置
    if(-1 != *nIndex)
    {
        *nLine = nLineTemp;
        return true;
    }
    return false;
}

bool AlgorithmHatching::getFirstLine_Greed(const int &nX, const int &nY, int *nLine, int *nIndex, TOTALHATCHINGLINE &lpListHLine, bool &bFirstLine)
{
    if(bFirstLine)
    {
        bFirstLine = false;
        int nLineCnt = lpListHLine.count();
        for(int iLIndex = 0; iLIndex < nLineCnt; iLIndex ++)
        {
            if(lpListHLine.at(iLIndex).listLineCoor.count() > 0)
            {
                *nIndex = 0;
                *nLine = iLIndex;
                return true;
            }
        }
    }
    else
    {
        double fTempDis = 0;
        double fMinDis = 1E20;
        int nSLineCnt = lpListHLine.count();
        for(int iLine = 0; iLine < nSLineCnt; ++ iLine)
        {
            QVector<LINECOOR> *listLine = &lpListHLine[iLine].listLineCoor;
            int nPointCnt = listLine->count();
            for(int iPt = 0; iPt < nPointCnt; ++ iPt)
            {
                fTempDis = sqrt(pow(listLine->at(iPt).X1 - nX, 2) +
                                pow(listLine->at(iPt).Y1 - nY, 2));
                if(fTempDis < fMinDis)
                {
                    fMinDis = fTempDis;
                    *nLine = iLine;
                    *nIndex = iPt;
                }
            }
        }
        return true;
    }
    return false;
}
