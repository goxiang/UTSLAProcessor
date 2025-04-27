#include "algorithmbase.h"
#include "writeuff.h"

#include <QDebug>

/**
 * @brief 计算路径集合的面积列表
 * 
 * @param paths 输入路径集合
 * @param lpAreaList 输出参数,存储每条路径的面积
 * @return double 返回所有路径的总面积
 */
double AlgorithmBase::calcAreaList(const Paths &paths, QList<double> &lpAreaList)
{
    double fArea = 0.0;
    double fAreaSingle = 0.0;
    for(const auto &path : paths)
    {
        fAreaSingle = Area(path);
        fArea += fAreaSingle;
        lpAreaList << fAreaSingle;
    }
    return fArea;
}

/**
 * @brief 计算路径集合的边界矩形
 * 
 * @param paths 输入路径集合
 * @param bdRC 输出参数,合并后的边界矩形
 * @param listRc 可选输出参数,存储每条路径的边界矩形
 */
void AlgorithmBase::calcLimitXY(const Paths &paths, BOUNDINGRECT &bdRC, QList<BDRECTPTR> *listRc)
{
    // 空路径检查
    if(paths.size() < 1) return;

    // 遍历所有路径
    for(const auto &path : paths)
    {
        // 创建新的边界矩形智能指针
        BDRECTPTR rc(new BOUNDINGRECT());
        // 计算当前路径的边界
        calcLimitXY(path, rc);
        // 合并到总边界矩形
        bdRC.merge(rc);
        // 如果需要,保存当前边界矩形
        if(listRc) listRc->append(rc);
    }
}

/**
 * @brief 计算区域列表的边界矩形
 * 
 * @param listArea 输入区域信息指针列表
 * @param bdRC 输出参数,合并后的边界矩形
 * @param listRc 可选输出参数,存储每个区域的边界矩形
 */
void AlgorithmBase::calcLimitXY(const QList<AREAINFOPTR> &listArea, BOUNDINGRECT &bdRC, QList<BDRECTPTR> *listRc)
{
    if(listArea.size() < 1) return;

    for(const auto &area : listArea)
    {
        BDRECTPTR rc(new BOUNDINGRECT());
        calcLimitXY(area, rc);
        bdRC.merge(rc);
        if(listRc) listRc->append(rc);
    }
}

/**
 * @brief 计算单条路径的边界矩形
 * 
 * @param path 输入路径
 * @param rc 输出参数,边界矩形智能指针
 */
void AlgorithmBase::calcLimitXY(const Path &path, BDRECTPTR &rc)
{
    if(path.size() < 1) return;

    for(const auto &pt : path)
    {
        if(pt.X < rc->minX)  rc->minX = int(pt.X);
        if(pt.X > rc->maxX)  rc->maxX = int(pt.X);
        if(pt.Y < rc->minY)  rc->minY = int(pt.Y);
        if(pt.Y > rc->maxY)  rc->maxY = int(pt.Y);
    }
}

/**
 * @brief 计算区域的边界矩形
 * 
 * @param area 输入区域信息指针
 * @param rc 输出参数,边界矩形智能指针
 */
void AlgorithmBase::calcLimitXY(const AREAINFOPTR &area, BDRECTPTR &rc)
{
    if(area->ptPath.size() < 1) return;

    for(const auto &pt : area->ptPath)
    {
        if(pt.X < rc->minX)  rc->minX = int(pt.X);
        if(pt.X > rc->maxX)  rc->maxX = int(pt.X);
        if(pt.Y < rc->minY)  rc->minY = int(pt.Y);
        if(pt.Y > rc->maxY)  rc->maxY = int(pt.Y);
    }
}

/**
 * @brief 获取所有区域的路径
 * 
 * @param listArea 输入区域信息指针列表
 * @param paths 输出参数,存储所有路径
 */
void AlgorithmBase::getAllPaths(const QList<AREAINFOPTR> &listArea, Paths &paths)
{
    paths.clear();
    for(const auto &area : listArea)
    {   
        // 使用移动语义将区域路径添加到输出容器
        paths << std::move(area->ptPath);
    }
}

/**
 * @brief 判断线段是否与矩形相交
 * 
 * @param X1 线段起点X坐标
 * @param Y1 线段起点Y坐标 
 * @param X2 线段终点X坐标
 * @param Y2 线段终点Y坐标
 * @param rcPtr 矩形边界智能指针
 * @return bool 相交返回true,不相交返回false
 * 
 * @details 相交判断流程:
 * 1. 计算线段方向向量
 * 2. 检查与矩形对角线1的相交情况:
 *    - 获取对角线端点坐标
 *    - 计算叉积判别式
 *    - 判别式<=0表示相交
 * 3. 检查与矩形对角线2的相交情况:
 *    - 获取对角线端点坐标
 *    - 计算叉积判别式
 *    - 判别式<=0表示相交
 */
bool AlgorithmBase::lineCrossRC(const double &X1, const double &Y1, const double &X2,
    const double &Y2, const BDRECTPTR &rcPtr)
{
// 计算线段方向向量
double fDelta_X12 = X1 - X2;
double fDelta_Y12 = Y1 - Y2;

double fDelta = 0;
double LX1 = 0.0, LY1 = 0.0, LX2 = 0.0, LY2 = 0.0;

// 检查与矩形对角线1的相交
LX1 = rcPtr->minX;
LY1 = rcPtr->minY;
LX2 = rcPtr->maxX;
LY2 = rcPtr->maxY;
fDelta = (fDelta_X12 * (LY1 - Y2) - fDelta_Y12 * (LX1 - X2)) *
(fDelta_X12 * (LY2 - Y2) - fDelta_Y12 * (LX2 - X2));
if(fDelta <= 0) return true;

// 检查与矩形对角线2的相交
LX1 = rcPtr->maxX;
LY1 = rcPtr->minY;
LX2 = rcPtr->minX; 
LY2 = rcPtr->maxY;
fDelta = (fDelta_X12 * (LY1 - Y2) - fDelta_Y12 * (LX1 - X2)) *
(fDelta_X12 * (LY2 - Y2) - fDelta_Y12 * (LX2 - X2));
if(fDelta <= 0) return true;

return false;
}


//void AlgorithmBase::sortPtOnLine(int nSortType, QList<EXPECTPTINFOPTR> &listExpectPt)
//{
//    int nPtCnt = listExpectPt.count();
//    switch(nSortType)
//    {
//    case SORTTYPE_X:
//        for(int iLine = 0; iLine < nPtCnt - 1; iLine ++)
//        {
//            for(int jLine = 0; jLine < nPtCnt - iLine - 1; jLine ++)
//            {
//                if(listExpectPt.at(jLine)->X > listExpectPt.at(jLine + 1)->X)
//                {
//                    listExpectPt.move(jLine + 1, jLine);
//                }
//            }
//        }
//        break;
//    case SORTTYPE_Y:
//        for(int iLine = 0; iLine < nPtCnt - 1; iLine ++)
//        {
//            for(int jLine = 0; jLine < nPtCnt - iLine - 1; jLine ++)
//            {
//                if(listExpectPt.at(jLine)->Y > listExpectPt.at(jLine + 1)->Y)
//                {
//                    listExpectPt.move(jLine + 1, jLine);
//                }
//            }
//        }
//        break;
//    default:
//        for(int iLine = 0; iLine < nPtCnt - 1; iLine ++)
//        {
//            for(int jLine = 0; jLine < nPtCnt - iLine - 1; jLine ++)
//            {
//                if(listExpectPt.at(jLine)->Y > listExpectPt.at(jLine + 1)->Y)
//                {
//                    listExpectPt.move(jLine + 1, jLine);
//                }
//            }
//        }
//        break;
//    }
//}

bool lessExpectX(const EXPECTPOINTINFO &, const EXPECTPOINTINFO &);
bool lessExpectY(const EXPECTPOINTINFO &, const EXPECTPOINTINFO &);

bool lessExpectX(const EXPECTPOINTINFO &expectPt1, const EXPECTPOINTINFO &expectPt2)
{
    return expectPt1.X < expectPt2.X;
}

bool lessExpectY(const EXPECTPOINTINFO &expectPt1, const EXPECTPOINTINFO &expectPt2)
{
    return expectPt1.Y < expectPt2.Y;
}

/**
 * @brief 对点集合按指定方向排序(QList版本)
 * 
 * @param nSortType 排序类型(SORTTYPE_X:按X排序, SORTTYPE_Y:按Y排序)
 * @param listExpectPt 待排序的点集合(QList容器)
 */
void AlgorithmBase::sortPtOnLine(int nSortType, QList<EXPECTPOINTINFO> &listExpectPt)
{
    switch(nSortType)
    {
    case SORTTYPE_X:
        std::sort(listExpectPt.begin(), listExpectPt.end(), lessExpectX);
        break;
    case SORTTYPE_Y:
        std::sort(listExpectPt.begin(), listExpectPt.end(), lessExpectY);
        break;
    default:
        std::sort(listExpectPt.begin(), listExpectPt.end(), lessExpectY);
        break;
    }
}

/**
 * @brief 对点集合按指定方向排序(QVector版本)
 * 
 * @param nSortType 排序类型(SORTTYPE_X:按X排序, SORTTYPE_Y:按Y排序)
 * @param listExpectPt 待排序的点集合(QVector容器)
 */
void AlgorithmBase::sortPtOnLine(int nSortType, QVector<EXPECTPOINTINFO> &listExpectPt)
{
    switch(nSortType)
    {
    case SORTTYPE_X:
        std::sort(listExpectPt.begin(), listExpectPt.end(), lessExpectX);
        break;
    case SORTTYPE_Y:
        std::sort(listExpectPt.begin(), listExpectPt.end(), lessExpectY);
        break;
    default:
        std::sort(listExpectPt.begin(), listExpectPt.end(), lessExpectY);
        break;
    }
}

/**
 * @brief 路径内部简化处理
 * 
 * @param paths_target 输入输出参数,待处理的路径集合
 * 
 * @details 简化处理流程:
 * 1. 初始化:
 *    - 检查路径数量
 *    - 分配路径使用标记数组
 *    - 初始化第一条路径
 * 
 * 2. 主循环处理:
 *    - 遍历未使用的路径
 *    - 检查与已有外轮廓的包含关系:
 *      * 被包含: 标记使用
 *      * 包含: 替换外轮廓并标记使用
 *    - 无包含关系时添加新外轮廓
 * 
 * 3. 清理和更新:
 *    - 释放标记数组
 *    - 用处理后的路径更新原路径集合
 */
void AlgorithmBase::reducePaths_Inner(Paths &paths_target)
{
    // 获取路径数量
    uint nPathCnt = paths_target.size();
    if(nPathCnt < 1) return;

    // 初始化参数
    PATHPARAMETERS tempPathPara;
    OULTLINEPARAMETERS tempOutLine;
    QList<OULTLINEPARAMETERS> mOutLines;
    Paths mOutPaths;

    // 初始化计数和标记
    uint nUsedCnt = 0;
    int nResult = 0;
    int *bPathUsed = static_cast<int *>(calloc(nPathCnt, sizeof(int)));
    bool bNOutLine = false;

    // 处理第一条路径
    bPathUsed[0] = 1;
    tempPathPara.initPath(paths_target.at(0));
    tempOutLine.topPath = tempPathPara;
    mOutLines << tempOutLine;
    mOutPaths << paths_target.at(0);// 保存第一条路径为外轮廓
    nUsedCnt = 1;

    // 处理剩余路径
    while(nUsedCnt < nPathCnt)
    {
        bNOutLine = true;
        // 遍历未使用的路径
        for(uint iSubPath = 1; iSubPath < nPathCnt; iSubPath ++)
        {
            if(!bPathUsed[iSubPath])
            {
                // 检查与已有外轮廓的关系
                for(uint iOutLine = 0; iOutLine < mOutPaths.size(); iOutLine ++)
                {
                    nResult = Poly2ContainsPoly1(paths_target.at(iSubPath), mOutPaths.at(iOutLine));
                    if(1 == nResult) // 被包含
                    {
                        bNOutLine = false;
                        nUsedCnt ++;
                        bPathUsed[iSubPath] = 1;
                    }
                    else if(-1 == nResult) // 包含
                    {
                        bNOutLine = false;
                        nUsedCnt ++;
                        bPathUsed[iSubPath] = 1;
                        mOutPaths.operator [](iOutLine) = paths_target.at(iSubPath);
                    }
                }
            }
        }

        // 添加新外轮廓（待处理的路径与所有已有外轮廓都不存在包含关系）
        if(bNOutLine)
        {
            for(uint iSubPath = 1; iSubPath < nPathCnt; iSubPath ++)
            {
                if(!bPathUsed[iSubPath])
                {
                    nUsedCnt ++;
                    bPathUsed[iSubPath] = 1;
                    mOutPaths << paths_target.at(iSubPath);
                    break;
                }
            }
        }
    }

    // 清理和更新
    free(bPathUsed);
    paths_target.clear();
    paths_target = mOutPaths;
}


/**
 * @brief 获取表面路径
 * 
 * @param targetPaths 输出参数,目标路径集合
 * @param nSurfaceIndex 输出参数,表面索引
 * @param curPaths 当前路径集合
 * @param lpPaths 路径数组指针
 * @param nSmallWall 小壁厚度
 * @param nSurfaceCnt 表面数量
 * 
 * @details 处理流程:
 * 1. 多表面处理(nSurfaceCnt > 1):
 *    - 从后向前遍历表面
 *    - 计算相邻表面差集
 *    - 计算与当前路径差集
 *    - 进行偏移处理验证
 * 
 * 2. 单表面处理:
 *    - 计算与当前路径差集
 *    - 进行偏移处理验证
 * 
 * 3. 结果处理:
 *    - 验证失败时清空结果
 *    - 更新表面索引
 */
void AlgorithmBase::getSurfacePaths(Paths &targetPaths, int &nSurfaceIndex, Paths *curPaths,
                                   Paths *lpPaths[], const int &nSmallWall, const int &nSurfaceCnt)
{
    // 多表面处理
    if(nSurfaceCnt > 1)
    {
        // 从后向前遍历表面
        for(int iSur = nSurfaceCnt - 1; iSur > 0; -- iSur)
        {
            if(lpPaths[iSur - 1] && lpPaths[iSur])
            {
                // 计算相邻表面差集
                getDifferencePaths(lpPaths[iSur - 1], lpPaths[iSur], targetPaths, nSmallWall);
                if(targetPaths.size())
                {
                    nSurfaceIndex = iSur;
                    // 计算与当前路径差集
                    getDifferencePaths(curPaths, lpPaths[iSur], targetPaths, nSmallWall);
                    if(targetPaths.size())
                    {
                        nSurfaceIndex = iSur;
                        Paths paths_Reserve;
                        getDifferencePaths(curPaths, &targetPaths, paths_Reserve, nSmallWall);
                        if(0 == paths_Reserve.size())
                        {
                            return;
                        }
                        // 偏移处理验证
                        getOffsetPaths(&targetPaths, &paths_Reserve, 0, FILEDATAUNIT, 0);
                        if(0 == paths_Reserve.size())
                        {
                            targetPaths.clear();
                            nSurfaceIndex = -1;
                        }
                        return;
                    }
                }
            }
        }

        // 处理第一个表面
        getDifferencePaths(curPaths, lpPaths[0], targetPaths, nSmallWall);
        if(targetPaths.size())
        {
            nSurfaceIndex = 0;
            Paths paths_Reserve;
            getDifferencePaths(curPaths, &targetPaths, paths_Reserve, nSmallWall);
            if(0 == paths_Reserve.size())
            {
                return;
            }
            getOffsetPaths(&targetPaths, &paths_Reserve, 0, FILEDATAUNIT, 0);
            if(0 == paths_Reserve.size())
            {
                targetPaths.clear();
                nSurfaceIndex = -1;
            }
            return;
        }
    }
    // 单表面处理
    else
    {
        getDifferencePaths(curPaths, lpPaths[0], targetPaths, nSmallWall);
        if(targetPaths.size())
        {
            nSurfaceIndex = 0;
            Paths paths_Reserve;
            getDifferencePaths(curPaths, &targetPaths, paths_Reserve, nSmallWall);
            if(0 == paths_Reserve.size())
            {
                return;
            }
            getOffsetPaths(&targetPaths, &paths_Reserve, 0, FILEDATAUNIT, 0);
            if(0 == paths_Reserve.size())
            {
                targetPaths.clear();
                nSurfaceIndex = -1;
                return;
            }
            return;
        }
    }

    // 处理失败,清空结果
    targetPaths.clear();
    nSurfaceIndex = -1;
}


/**
 * @brief 计算路径集合与单条路径的差集
 * 
 * @param curPaths 输入参数,当前路径集合
 * @param subPath 输入参数,需要减去的单条路径
 * @param targetPaths 输出参数,差集结果路径
 * @param nSmallWall 小壁厚度,用于偏移处理
 * 
 * @details 差集计算流程:
 * 1. 使用Clipper进行布尔差集运算:
 *    - 添加当前路径集合作为主体
 *    - 添加单条路径作为裁剪对象
 *    - 执行差集运算
 *    - 清理结果多边形
 * 
 * 2. 小壁处理(当nSmallWall>0):
 *    - 先进行负向偏移
 *    - 再进行正向偏移
 *    - 每次偏移后清理多边形
 */
void AlgorithmBase::getDifferencePaths(const Paths &curPaths, const Path &subPath, 
                                     Paths &targetPaths, const int &nSmallWall)
{
    // 创建Clipper对象进行布尔运算
    Clipper c;
    c.AddPaths(curPaths, ptSubject, true);
    c.AddPath(subPath, ptClip, true);
    c.Execute(ctDifference, targetPaths, pftEvenOdd, pftEvenOdd);
    CleanPolygons(targetPaths, MINDISTANCE);

    // 小壁偏移处理（去除小尖角）
    if(targetPaths.size() && nSmallWall > 0)
    {
        // 负向偏移
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(targetPaths, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(targetPaths, - nSmallWall);
            CleanPolygons(targetPaths, MINDISTANCE);
        }
        // 正向偏移
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(targetPaths, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(targetPaths, nSmallWall);
            CleanPolygons(targetPaths, MINDISTANCE);
        }
    }
}


void AlgorithmBase::getDifferencePaths(const Paths *curPaths, const Paths *subPaths, Paths &targetPaths, const int &nSmallWall)
{
    Clipper c;
    c.AddPaths(*curPaths, ptSubject, true);
    c.AddPaths(*subPaths, ptClip, true);
    c.Execute(ctDifference, targetPaths, pftEvenOdd, pftEvenOdd);
    CleanPolygons(targetPaths, MINDISTANCE);
    
    // 移除小面积多边形
    // uint iPath = 0;
    // auto fMinArea = pow(0.17 * UNITSPRECISION, 2);
    // bool needSimplify = false;
    // while(iPath < targetPaths.size())
    // {
    //     if(fabs(Area(targetPaths.at(iPath))) < fMinArea)
    //     {
    //         targetPaths.erase(targetPaths.begin() + int(iPath));
    //         needSimplify = true;
    //         continue;
    //     }
    //     ++ iPath;
    // }
    // if(needSimplify) SimplifyPolygons(targetPaths);

    if(targetPaths.size() && nSmallWall > 0)
    {
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(targetPaths, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(targetPaths, - nSmallWall);
            CleanPolygons(targetPaths, MINDISTANCE);
        }
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(targetPaths, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(targetPaths, nSmallWall);
            CleanPolygons(targetPaths, MINDISTANCE);
        }
    }
}

///
/// @brief 计算两个路径的交集并进行偏移处理
/// @param curPath 当前路径
/// @param subPath 子路径
/// @param targetPaths 输出的目标路径集合
/// @param nSmallWall 偏移距离
/// @details 实现步骤:
///   1. 计算两个路径的交集
///   2. 清理多边形
///   3. 如果有偏移要求:
///      - 先执行负向偏移
///      - 再执行正向偏移
///   4. 每次偏移后清理多边形
///
void AlgorithmBase::getUnionPaths(const Path &curPath, const Path &subPath, 
                                 Paths &targetPaths, const int &nSmallWall)
{
    // 创建裁剪器对象
    Clipper c;
    // 添加当前路径作为主要路径
    c.AddPath(curPath, ptSubject, true);
    // 添加子路径作为裁剪路径
    c.AddPath(subPath, ptClip, true);
    // 执行交集运算
    c.Execute(ctIntersection, targetPaths, pftEvenOdd, pftEvenOdd);
    // 清理多边形,去除微小缺陷
    CleanPolygons(targetPaths, MINDISTANCE);

    // 如果有路径且需要偏移处理
    if(targetPaths.size() && nSmallWall > 0)
    {
        // 执行负向偏移
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(targetPaths, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(targetPaths, - nSmallWall);
            CleanPolygons(targetPaths, MINDISTANCE);
        }
        // 执行正向偏移
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(targetPaths, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(targetPaths, nSmallWall);
            CleanPolygons(targetPaths, MINDISTANCE);
        }
    }
}


void AlgorithmBase::getUnionPaths(const Path &curPath, const Paths &subPaths, Paths &targetPaths, const int &nSmallWall)
{
    Clipper c;
    c.AddPath(curPath, ptSubject, true);
    c.AddPaths(subPaths, ptClip, true);
    c.Execute(ctIntersection, targetPaths, pftEvenOdd, pftEvenOdd);
    CleanPolygons(targetPaths, MINDISTANCE);
    if(targetPaths.size() && nSmallWall > 0)
    {
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(targetPaths, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(targetPaths, - nSmallWall);
            CleanPolygons(targetPaths, MINDISTANCE);
        }
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(targetPaths, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(targetPaths, nSmallWall);
            CleanPolygons(targetPaths, MINDISTANCE);
        }
    }
}

void AlgorithmBase::getUnionPaths(const Paths *curPaths, const Paths *subPaths, Paths &targetPaths, const int &nSmallWall)
{
    Clipper c;
    c.AddPaths(*curPaths, ptSubject, true);
    c.AddPaths(*subPaths, ptClip, true);
    c.Execute(ctIntersection, targetPaths, pftEvenOdd, pftEvenOdd);
    CleanPolygons(targetPaths, MINDISTANCE);

    // uint iPath = 0;
    // auto fMinArea = pow(0.17 * UNITSPRECISION, 2);
    // bool needSimplify = false;
    // while(iPath < targetPaths.size())
    // {
    //     if(fabs(Area(targetPaths.at(iPath))) < fMinArea)
    //     {
    //         targetPaths.erase(targetPaths.begin() + int(iPath));
    //         needSimplify = true;
    //         continue;
    //     }
    //     ++ iPath;
    // }
    // if(needSimplify) SimplifyPolygons(targetPaths);

    if(targetPaths.size() && nSmallWall > 0)
    {
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(targetPaths, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(targetPaths, - nSmallWall);
            CleanPolygons(targetPaths, MINDISTANCE);
        }
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(targetPaths, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(targetPaths, nSmallWall);
            CleanPolygons(targetPaths, MINDISTANCE);
        }
    }
}

void AlgorithmBase::getOffsetPaths(Paths *lpSrc, int nOffset, int nReserved, int nSmallWall)
{
    getOffsetPaths(lpSrc, lpSrc, nOffset, nReserved, nSmallWall);
}

/**
 * @brief 路径偏移处理(双参数版本)
 * 
 * @param lpSrc 输入参数,源路径指针
 * @param lpTar 输出参数,目标路径指针
 * @param nOffset 偏移距离
 * @param nReserved 保留距离
 * @param nSmallWall 小壁厚度
 * 
 * @details 偏移处理流程:
 * 1. 有保留距离时:
 *    - 先进行负向偏移(保留距离+小壁)
 *    - 再进行正向偏移(保留距离+小壁-偏移距离)
 * 
 * 2. 无保留距离时:
 *    - 直接进行负向偏移
 */
void AlgorithmBase::getOffsetPaths(Paths *lpSrc, Paths *lpTar, const double &nOffset,
                                  const double &nReserved, const double &nSmallWall)
{
    // 有保留距离的处理
    if(nReserved > 0)
    {
        // 负向偏移
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(*lpSrc, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(*lpTar, - nReserved - nSmallWall);
            CleanPolygons(*lpTar, MINDISTANCE);
        }
        // 正向偏移
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(*lpTar, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(*lpTar, nReserved + nSmallWall - nOffset);
            CleanPolygons(*lpTar, MINDISTANCE);
        }
    }
    // 无保留距离的处理
    else
    {
        // 直接负向偏移
        {
            ClipperOffset cliper_Offset;
            cliper_Offset.AddPaths(*lpSrc, jtMiter, etClosedPolygon);
            cliper_Offset.Execute(*lpTar, - nOffset);
            CleanPolygons(*lpTar, MINDISTANCE);
        }
    }
}

void AlgorithmBase::getIntersectionPaths(const Paths &curPaths, const Path &subPaths, Paths &targetPaths)
{
    Clipper c;
    c.AddPaths(curPaths, ptSubject, true);
    c.AddPath(subPaths, ptClip, true);
    c.Execute(ctIntersection, targetPaths, pftEvenOdd, pftEvenOdd);
    CleanPolygons(targetPaths, MINDISTANCE);
}

void AlgorithmBase::getIntersectionPaths(const Paths &curPaths, const Paths &subPaths, Paths &targetPaths)
{
    Clipper c;
    c.AddPaths(curPaths, ptSubject, true);
    c.AddPaths(subPaths, ptClip, true);
    c.Execute(ctIntersection, targetPaths, pftEvenOdd, pftEvenOdd);
    CleanPolygons(targetPaths, MINDISTANCE);
}

/**
 * @brief 添加上层路径
 * 
 * @param path_target 输入输出参数,目标路径集合
 * @param path_refer 输入参数,参考路径集合指针
 * 
 * @details 处理流程:
 * 1. 计算边界矩形:
 *    - 定义PathRect结构存储边界信息
 *    - 分别计算参考路径和目标路径的边界矩形
 * 
 * 2. 路径比较:
 *    - 检查边界矩形是否相似(误差在FILEDATAUNIT_0_05内)
 *    - 对于不相似的路径,检查包含关系
 *    - 符合条件的路径索引保存到listIndex
 * 
 * 3. 添加路径:
 *    - 将符合条件的参考路径添加到目标路径集合
 */
void AlgorithmBase::addPaths_Upper(Paths &path_target, Paths *path_refer)
{
    // 获取路径数量
    size_t nReferCnt = path_refer->size();
    size_t nTargetCnt = path_target.size();
    QList<int> listIndex;

    // 定义边界矩形结构
    struct PathRect{
        IntPoint minBounder;
        IntPoint maxBounder;

        PathRect() {
            minBounder.X = hiRange;
            minBounder.Y = hiRange;
            maxBounder.X = 0;
            maxBounder.Y = 0;
        }
    };

    // 计算参考路径边界矩形
    QList<PathRect> listPathRectRefer, listPathRectTarget;
    Path *pathTemp = nullptr;
    size_t nPtCnt = 0;
    for(size_t iRefer = 0; iRefer < nReferCnt; iRefer ++)
    {
        PathRect pathRect;
        pathTemp = &path_refer->at(iRefer);
        nPtCnt = pathTemp->size();
        // 遍历路径点更新边界
        for(size_t iPt = 0; iPt < nPtCnt; ++iPt)
        {
            // 更新X边界
            if(pathTemp->at(iPt).X < pathRect.minBounder.X)
                pathRect.minBounder.X = pathTemp->at(iPt).X;
            if(pathTemp->at(iPt).X > pathRect.maxBounder.X)
                pathRect.maxBounder.X = pathTemp->at(iPt).X;
            // 更新Y边界    
            if(pathTemp->at(iPt).Y < pathRect.minBounder.Y)
                pathRect.minBounder.Y = pathTemp->at(iPt).Y;
            if(pathTemp->at(iPt).Y > pathRect.maxBounder.Y)
                pathRect.maxBounder.Y = pathTemp->at(iPt).Y;
        }
        listPathRectRefer << pathRect;
    }

    // 计算目标路径边界矩形
    for(size_t iTarget = 0; iTarget < nTargetCnt; iTarget ++)
    {
        PathRect pathRect;
        pathTemp = &path_target.at(iTarget);
        nPtCnt = pathTemp->size();
        // 遍历路径点更新边界
        for(size_t iPt = 0; iPt < nPtCnt; ++iPt)
        {
            // 更新X边界
            if(pathTemp->at(iPt).X < pathRect.minBounder.X)
                pathRect.minBounder.X = pathTemp->at(iPt).X;
            if(pathTemp->at(iPt).X > pathRect.maxBounder.X)
                pathRect.maxBounder.X = pathTemp->at(iPt).X;
            // 更新Y边界
            if(pathTemp->at(iPt).Y < pathRect.minBounder.Y)
                pathRect.minBounder.Y = pathTemp->at(iPt).Y;
            if(pathTemp->at(iPt).Y > pathRect.maxBounder.Y)
                pathRect.maxBounder.Y = pathTemp->at(iPt).Y;
        }
        listPathRectTarget << pathRect;
    }

    // 比较路径并添加符合条件的索引
    PathRect *lpPathRect = nullptr;
    bool bFindSimilar = false;
    for(int iRefer = 0; iRefer < int(nReferCnt); iRefer ++)
    {
        bFindSimilar = false;
        lpPathRect = &listPathRectRefer[iRefer];
        // 检查是否存在相似路径
        for(int iTarget = 0; iTarget < int(nTargetCnt); ++ iTarget)
        {
            if(abs(lpPathRect->minBounder.X - listPathRectTarget.at(iTarget).minBounder.X) < FILEDATAUNIT_0_05 &&
                    abs(lpPathRect->minBounder.Y - listPathRectTarget.at(iTarget).minBounder.Y) < FILEDATAUNIT_0_05 &&
                    abs(lpPathRect->maxBounder.X - listPathRectTarget.at(iTarget).maxBounder.X) < FILEDATAUNIT_0_05  &&
                    abs(lpPathRect->maxBounder.Y - listPathRectTarget.at(iTarget).maxBounder.Y) < FILEDATAUNIT_0_05 )
            {
                bFindSimilar = true;
                break;
            }
        }

        // 检查不相似路径的包含关系
        if(false == bFindSimilar)
        {
            for(int iTarget = 0; iTarget < int(nTargetCnt); iTarget ++)
            {
                if((lpPathRect->minBounder.X - listPathRectTarget.at(iTarget).minBounder.X) > FILEDATAUNIT_0_05 &&
                        (lpPathRect->minBounder.Y - listPathRectTarget.at(iTarget).minBounder.Y) > FILEDATAUNIT_0_05 &&
                        (listPathRectTarget.at(iTarget).maxBounder.X - lpPathRect->maxBounder.X) > FILEDATAUNIT_0_05 &&
                        (listPathRectTarget.at(iTarget).maxBounder.Y - lpPathRect->maxBounder.Y) > FILEDATAUNIT_0_05 &&
                        (1 == Poly2ContainsPoly1(path_refer->at(uint(iRefer)), path_target.at(uint(iTarget)))))
                {
                    listIndex << iRefer;
                }
            }
        }
    }

    // 添加符合条件的路径
    int nListCnt = listIndex.count();
    if(nListCnt)
    {
        for(int iIndex = 0; iIndex < nListCnt; iIndex ++)
        {
            path_target << path_refer->at(uint(listIndex.at(iIndex)));
        }
    }
}


/**
 * @brief 扩展边界路径
 * 
 * @param path_refer 输入参数,参考路径集合指针
 * @param path_Src 输入参数,源路径集合指针
 * @param extendBorders 输出参数,扩展边界路径集合指针
 * 
 * @details 处理流程:
 * 1. 初始化路径参数:
 *    - 分别计算源路径和参考路径的参数信息
 *    - 使用PATHPARAMETERS存储路径特征
 * 
 * 2. 路径比较和扩展:
 *    - 遍历参考路径集合
 *    - 与源路径集合比较,找出不重复的路径
 *    - 将不重复的路径添加到扩展边界中
 */
void AlgorithmBase::extendBorders(Paths *path_refer, Paths *path_Src, Paths *extendBorders)
{
    // 获取路径数量
    uint srcCnt = path_Src->size();
    uint refCnt = path_refer->size();
    
    // 初始化路径参数列表
    QList<PATHPARAMETERS> list_SrcBorderInfo;
    QList<PATHPARAMETERS> list_refBorderInfo;
    PATHPARAMETERS tempContourInfo;

    // 计算源路径参数
    for(uint iPathSrc = 0; iPathSrc < srcCnt; ++ iPathSrc)
    {
        tempContourInfo.initPath(path_Src->at(iPathSrc));
        list_SrcBorderInfo << tempContourInfo;
    }

    // 计算参考路径参数
    for(uint iPathRef = 0; iPathRef < refCnt; ++ iPathRef)
    {
        tempContourInfo.initPath(path_refer->at(iPathRef));
        list_refBorderInfo << tempContourInfo;
    }

    // 比较路径并扩展边界
    bool bAppend = true;
    for(uint iPathRef = 0; iPathRef < refCnt; ++ iPathRef)
    {
        bAppend = true;
        // 检查是否存在重复路径
        for(uint iPathSrc = 0; iPathSrc < srcCnt; ++ iPathSrc)
        {
            if(list_refBorderInfo.at(int(iPathRef)) == list_SrcBorderInfo.at(int(iPathSrc)))
            {
                bAppend = false;
                break;
            }
        }
        // 添加不重复的路径
        if(bAppend)
        {
            *extendBorders << path_refer->at(iPathRef);
        }
    }
}


/**
 * @brief 添加跳转扫描线
 * 
 * @param X X坐标
 * @param Y Y坐标
 * @param listSLines 扫描线列表
 * 
 * @details 创建一条跳转类型(JUMP)的扫描线并添加到列表中
 */
void AlgorithmBase::scanner_jumpPos(const int &X, const int &Y, QVector<SCANLINE> &listSLines)
{
    SCANLINE tempScanLine;
    tempScanLine.nLineType = SECTION_SCANTYPE_JUMP; // 设置为跳转类型
    tempScanLine.nX = X;
    tempScanLine.nY = Y;
    listSLines << tempScanLine;
}

/**
 * @brief 添加标记扫描线
 * 
 * @param X X坐标
 * @param Y Y坐标
 * @param listSLines 扫描线列表
 * 
 * @details 创建一条标记类型(MARK)的扫描线并添加到列表中
 */
void AlgorithmBase::scanner_outLine(const int &X, const int &Y, QVector<SCANLINE> &listSLines)
{
    SCANLINE tempScanLine;
    tempScanLine.nLineType = SECTION_SCANTYPE_MARK; // 设置为标记类型
    tempScanLine.nX = X;
    tempScanLine.nY = Y;
    listSLines << tempScanLine;
}


void AlgorithmBase::scanner_jumpPos(const qint64 &X, const qint64 &Y, QVector<SCANLINE> &listSLines)
{
    scanner_jumpPos(int(X), int(Y), listSLines);
}

void AlgorithmBase::scanner_outLine(const qint64 &X, const qint64 &Y, QVector<SCANLINE> &listSLines)
{
    scanner_outLine(int(X), int(Y), listSLines);
}

/**
 * @brief 生成一条完整的扫描线段
 * 
 * @param X1 起点X坐标
 * @param Y1 起点Y坐标
 * @param X2 终点X坐标
 * @param Y2 终点Y坐标
 * @param listSLines 扫描线列表
 * 
 * @details 生成流程:
 * 1. 添加起点跳转扫描线(JUMP)
 * 2. 添加终点标记扫描线(MARK)
 */
void AlgorithmBase::scanner_plotLine(const int &X1, const int &Y1, const int &X2,
                                   const int &Y2, QVector<SCANLINE> &listSLines)
{
    // 添加起点跳转线
    SCANLINE tempScanLine;
    tempScanLine.nLineType = SECTION_SCANTYPE_JUMP;
    tempScanLine.nX = X1;
    tempScanLine.nY = Y1;
    listSLines << tempScanLine;

    // 添加终点标记线
    SCANLINE markLine;
    markLine.nLineType = SECTION_SCANTYPE_MARK;
    markLine.nX = X2;
    markLine.nY = Y2;
    listSLines << markLine;
}


/**
 * @brief 检查线段是否在拼接线上
 * 
 * @param x1 线段起点X坐标
 * @param y1 线段起点Y坐标
 * @param x2 线段终点X坐标
 * @param y2 线段终点Y坐标
 * @param vecSplicingLine 拼接线向量
 * @return true 线段在拼接线上
 * @return false 线段不在拼接线上
 * 
 * @details 检查流程:
 * 1. 遍历所有拼接线
 * 2. 检查线段两端点是否在拼接位置附近(误差范围内)
 * 3. 如果在拼接位置,检查是否包含该线段
 */
bool AlgorithmBase::lineOnVecSplicingLine(int64_t x1, int64_t y1, int64_t x2, int64_t y2,
                                         QVector<SplicingLine> &vecSplicingLine)
{
    // 遍历拼接线
    for(const auto &splicingLine : vecSplicingLine)
    {
        // 检查线段端点是否在拼接位置
        if((abs(x1 - splicingLine.nSplicingPos) < SPLICING_ERROR_HOR) &&
           (abs(x2 - splicingLine.nSplicingPos) < SPLICING_ERROR_HOR))
        {
            // 检查是否包含该线段
            if(splicingLine.vecLine.contains(LINENORMAL(x1, y1, x2, y2)))
            {
                return true;
            }
            return false;
        }
    }
    return true;
}


/**
 * @brief 过滤小边界路径
 * 
 * @param src 输入参数,源路径集合
 * @param remained 输出参数,保留的路径集合
 * @param filteOut 输出参数,被过滤出的小边界路径集合
 * @param areaLimit 面积阈值
 * 
 * @details 过滤流程:
 * 1. 遍历源路径集合
 * 2. 计算每条路径的面积
 * 3. 根据面积阈值分类:
 *    - 面积大于0且小于阈值的路径添加到filteOut
 *    - 其他路径添加到remained
 */
void AlgorithmBase::filteSmallBorder(const Paths &src, Paths &remained, Paths &filteOut,
                                    const double &areaLimit)
{
    uint nTempIndex = 0;
    double fArea = 0;
    
    // 遍历源路径
    while(nTempIndex < src.size())
    {
        // 计算路径面积
        fArea = Area(src[nTempIndex]);
        
        // 根据面积分类
        if(fArea > 0 && fArea < areaLimit)
        {
            filteOut << src[nTempIndex];  // 添加到过滤集合
        }
        else
        {
            remained << src[nTempIndex];   // 添加到保留集合
        }

        ++ nTempIndex;
    }
}

/**
 * @brief 将扫描数据写入列表
 * 
 * @param nLastX 上一点X坐标
 * @param nLastY 上一点Y坐标  
 * @param nCurX 当前点X坐标
 * @param nCurY 当前点Y坐标
 * @param nList_LastX 列表中最后一点X坐标
 * @param nList_LastY 列表中最后一点Y坐标
 * @param listSLine 扫描线列表
 * 
 * @details 写入流程:
 * 1. 如果列表最后点与上一点不同:
 *    - 生成完整扫描线段(包含跳转和标记)
 * 2. 如果列表最后点与上一点相同:
 *    - 只添加标记线
 * 3. 更新列表最后点坐标
 */
void AlgorithmBase::writeDataToList(const int64_t &nLastX, const int64_t &nLastY, 
                                  const int64_t &nCurX, const int64_t &nCurY,
                                  int64_t &nList_LastX, int64_t &nList_LastY, 
                                  QVector<SCANLINE> &listSLine)
{
    // 检查是否需要生成完整线段
    if(nList_LastX != nLastX || nList_LastY != nLastY)
    {
        scanner_plotLine(int(nLastX), int(nLastY), int(nCurX), int(nCurY), listSLine);
    }
    else
    {
        scanner_outLine(int(nCurX), int(nCurY), listSLine);
    }

    // 更新列表最后点坐标
    nList_LastX = nCurX;
    nList_LastY = nCurY;
}

