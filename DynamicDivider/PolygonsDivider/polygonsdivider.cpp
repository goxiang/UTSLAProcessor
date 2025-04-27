#include "polygonsdivider.h"
#include "dividerheader.h"

#include <QElapsedTimer>
#include <list>

struct PolygonsDivider::Priv {
    long long _minX = std::numeric_limits<long long>::max();
    long long _minY = std::numeric_limits<long long>::max();
    long long _maxX = std::numeric_limits<long long>::min();
    long long _maxY = std::numeric_limits<long long>::min();
    std::list<EaringTriangle> _earingTriList;
    double _maxError = 10;

    Priv() = default;
    virtual ~Priv() {}
};

PolygonsDivider::PolygonsDivider() : _d(new Priv) { }

long long PolygonsDivider::calcWeightPos(const ClipperLib::Paths &in_polygons, const double &ratio,
                                         int &splitHorizon)
{
    createEaring(in_polygons);
    return getWeightPos(ratio, splitHorizon);
}

void PolygonsDivider::setMaxError(const double &maxError)
{
    _d->_maxError = maxError;
}

//Private
///
/// @brief 对多边形集合进行耳切三角剖分
/// @param polygons 输入多边形集合
/// @details 实现步骤:
///   1. 遍历每个多边形
///   2. 计算多边形面积
///   3. 根据面积正负判断是外轮廓还是内轮廓
///   4. 转换为点列表后进行相应的耳切处理
///
void PolygonsDivider::createEaring(const Paths &polygons)
{
    // 遍历多边形集合
    for (const auto &polygon : polygons)
    {
        // 计算当前多边形面积
        double fArea = Area(polygon);
        
        // 面积大于0表示外轮廓
        if (fArea > 0)
        {
            std::list<IntPoint> ptList;
            pathsToList(polygon, ptList);
            createEaring_outter(ptList);
        }
        // 面积小于0表示内轮廓
        else if (fArea < 0)
        {
            std::list<IntPoint> ptList;
            pathsToList(polygon, ptList);
            createEaring_inner(ptList);
        }
    }
}


long long PolygonsDivider::getWeightPos(const double &ratio, int &splitHorizon)
{
    MinErrResult result;

    if ((_d->_maxY - _d->_minY) >= (_d->_maxX - _d->_minX))
    {
        splitHorizon = 1;
        getWeightPos(ratio, _d->_minY, _d->_maxY, result);
    }
    else
    {
        splitHorizon = 0;
        getWeightPos_ver(ratio, _d->_minX, _d->_maxX, result);
    }

    if (result._error > 1E8) qDebug() << "getWeightPos" << ratio << result._pos << result._error << splitHorizon;
    return result._pos;
}

///
/// @brief 将多边形路径转换为点列表
/// @param polygon 输入多边形路径
/// @param ptList 输出参数,转换后的点列表
/// @details 实现步骤:
///   1. 清空输出列表
///   2. 检查输入多边形顶点数是否大于等于3
///   3. 将多边形的每个顶点依次添加到列表中
///
void PolygonsDivider::pathsToList(const Path &polygon,
                                  std::list<IntPoint> &ptList)
{
    // 清空输出列表
    ptList.clear();
    // 检查输入多边形是否有效
    if (polygon.size() < 3) return;

    // 将多边形顶点复制到列表中
    for (const auto &pt : polygon) ptList.push_back(pt);
}


///
/// @brief 处理外部轮廓的耳切三角剖分
/// @param ptList 输入的顶点列表，会在处理过程中被修改
/// @details 实现步骤:
///   1. 循环处理直到顶点数小于等于3
///   2. 每次选取连续三个顶点判断是否可以形成有效耳朵
///   3. 判断是否有其他顶点在待切除三角形内
///   4. 切除有效的耳朵并继续处理
///   5. 最后处理剩余的三个顶点
///
void PolygonsDivider::createEaring_outter(std::list<IntPoint> &ptList)
{
    auto it = ptList.cbegin();
    // 当顶点数大于3时继续处理
    while (ptList.size() > 3)
    {
        // 获取连续三个顶点的迭代器
        auto it2 = std::next(it);
        if (ptList.cend() == it2) it2 = ptList.cbegin();
        auto it3 = std::next(it2);
        if (ptList.cend() == it3) it3 = ptList.cbegin();

        // 计算三角形的方向(面积的两倍)
        auto dir = (it2->X - it->X) * (it3->Y - it2->Y) -
                   (it2->Y - it->Y) * (it3->X - it2->X);
        
        // 如果是顺时针方向
        if (dir > 0)
        {
            // 构造临时三角形并检测
            EaringTriangle tempTriangle(*it, *it2, *it3, dir * 0.5);
            bool ptInTriangle = false;
            
            // 检查是否有其他顶点在三角形内
            for (auto tempIt = ptList.cbegin(); tempIt != ptList.cend(); ++ tempIt)
            {
                if (tempIt == it) continue;
                if (tempIt == it2) continue;
                if (tempIt == it3) continue;

                if (false == tempTriangle.ptOutTheTriangle(*tempIt))
                {
                    ptInTriangle = true;
                    break;
                }
            }

            // 如果没有点在三角形内，则切除这个耳朵
            if (false == ptInTriangle)
            {
                appendEaringTriangle(tempTriangle);
                ptList.remove(*it2);
            }
            else it = it2;
        }
        // 如果三点共线，移除中间点
        else if (fabs(dir) < 1E-6) ptList.remove(*it2);
        else it = it2;
    }

    // 处理最后剩余的三个顶点
    if (3 == ptList.size())
    {
        it = ptList.cbegin();
        auto it2 = std::next(it);
        auto it3 = std::next(it2);
        auto dir = (it2->X - it->X) * (it3->Y - it2->Y) -
                   (it2->Y - it->Y) * (it3->X - it2->X);
        appendEaringTriangle(*it, *it2, *it3, dir);
    }
}


///
/// @brief 处理内部轮廓的耳切三角剖分
/// @param ptList 输入的顶点列表，会在处理过程中被修改
/// @details 实现步骤:
///   1. 循环处理直到顶点数小于等于3
///   2. 每次选取连续三个顶点判断是否可以形成有效耳朵
///   3. 判断是否有其他顶点在待切除三角形内
///   4. 切除有效的耳朵并继续处理
///   5. 最后处理剩余的三个顶点
///
void PolygonsDivider::createEaring_inner(std::list<IntPoint> &ptList)
{
    auto it = ptList.cbegin();
    // 当顶点数大于3时继续处理
    while (ptList.size() > 3)
    {
        // 获取连续三个顶点的迭代器
        auto it2 = std::next(it);
        if (ptList.cend() == it2) it2 = ptList.cbegin();
        auto it3 = std::next(it2);
        if (ptList.cend() == it3) it3 = ptList.cbegin();

        // 计算三角形的方向(面积的两倍)
        auto dir = (it2->X - it->X) * (it3->Y - it2->Y) -
                   (it2->Y - it->Y) * (it3->X - it2->X);
        
        // 如果是逆时针方向
        if (dir < 0)
        {
            // 构造临时三角形用于检测
            EaringTriangle tempTriangle(*it, *it3, *it2, 0.0);
            bool ptInTriangle = false;
            
            // 检查是否有其他顶点在三角形内
            for (auto tempIt = ptList.cbegin(); tempIt != ptList.cend(); ++ tempIt)
            {
                if (tempIt == it) continue;
                if (tempIt == it2) continue;
                if (tempIt == it3) continue;

                if (false == tempTriangle.ptOutTheTriangle(*tempIt))
                {
                    ptInTriangle = true;
                    break;
                }
            }

            // 如果没有点在三角形内，则切除这个耳朵
            if (false == ptInTriangle)
            {
                EaringTriangle triangle(*it, *it2, *it3, dir * 0.5);
                appendEaringTriangle(triangle);
                ptList.remove(*it2);
            }
            else it = it2;
        }
        // 如果三点共线，移除中间点
        else if (fabs(dir) < 1E-6) ptList.remove(*it2);
        else it = it2;
    }

    // 处理最后剩余的三个顶点
    if (3 == ptList.size())
    {
        it = ptList.cbegin();
        auto it2 = std::next(it);
        auto it3 = std::next(it2);
        auto dir = (it2->X - it->X) * (it3->Y - it2->Y) -
                   (it2->Y - it->Y) * (it3->X - it2->X);
        appendEaringTriangle(*it, *it2, *it3, dir);
    }
}


///
/// @brief 根据三个顶点和面积创建并添加耳切三角形
/// @param p1 第一个顶点坐标
/// @param p2 第二个顶点坐标
/// @param p3 第三个顶点坐标
/// @param area 输入面积值(实际使用其一半)
/// @details 实现步骤:
///   1. 使用三个顶点和面积构造耳切三角形
///   2. 更新边界框的最小最大坐标值
///   3. 将三角形添加到列表中
///
void PolygonsDivider::appendEaringTriangle(const IntPoint &p1,
                                           const IntPoint &p2,
                                           const IntPoint &p3, const double &area)
{
    // 构造耳切三角形,面积取输入值的一半
    EaringTriangle earingTriangle(p1, p2, p3, area * 0.5);

    // 更新边界框的最小X坐标
    _d->_minX = std::min(_d->_minX, earingTriangle._minX);
    // 更新边界框的最小Y坐标
    _d->_minY = std::min(_d->_minY, earingTriangle._minY);
    // 更新边界框的最大X坐标
    _d->_maxX = std::max(_d->_maxX, earingTriangle._maxX);
    // 更新边界框的最大Y坐标
    _d->_maxY = std::max(_d->_maxY, earingTriangle._maxY);

    // 将三角形移动添加到三角形列表中
    _d->_earingTriList.push_back(std::move(earingTriangle));
}


///
/// @brief 添加一个耳切三角形到三角形列表，同时更新边界框范围
/// @param triangle 待添加的耳切三角形
/// @details 实现步骤:
///   1. 更新边界框的最小最大坐标值
///   2. 将三角形添加到列表中
///
void PolygonsDivider::appendEaringTriangle(const EaringTriangle &triangle)
{
    // 更新边界框的最小X坐标
    _d->_minX = std::min(_d->_minX, triangle._minX);
    // 更新边界框的最小Y坐标  
    _d->_minY = std::min(_d->_minY, triangle._minY);
    // 更新边界框的最大X坐标
    _d->_maxX = std::max(_d->_maxX, triangle._maxX);
    // 更新边界框的最大Y坐标
    _d->_maxY = std::max(_d->_maxY, triangle._maxY);

    // 将三角形移动添加到三角形列表中
    _d->_earingTriList.push_back(std::move(triangle));
}

///
/// @brief 使用二分法计算最优水平分割位置以满足面积比例要求
/// @param ratio 目标面积比例
/// @param pos1 搜索范围起始Y坐标
/// @param pos2 搜索范围结束Y坐标
/// @param result 输出参数,记录最优分割结果
/// @details 实现步骤:
///   1. 检查搜索范围是否足够大
///   2. 计算中点位置
///   3. 统计完全在中点上下的三角形面积
///   4. 处理被中点分割的三角形
///   5. 计算加权面积差并更新结果
///   6. 根据面积差递归搜索更优位置
///
void PolygonsDivider::getWeightPos(const double &ratio, const long long &pos1, const long long &pos2,
                                   MinErrResult &result)
{
    // 搜索范围过小则返回
    if (pos2 - pos1 < 1000) return;

    // 存储被分割的三角形
    std::list<EaringTriangle> earingTriList;
    // 计算中点位置
    auto halfPos = pos1 + ((pos2 - pos1) >> 1);

    // 计算互补比例
    double r2 = 1 - ratio;
    // 记录上下部分面积
    double highAreas = 0.0;
    double lowAreas = 0.0;

    // 统计完全在中点上下的三角形面积
    for (const auto &it : _d->_earingTriList) {
        if (halfPos <= it._minY) highAreas += it._area;
        else if (halfPos >= it._maxY) lowAreas += it._area;
        else earingTriList.push_back(it);
    }

    {
        // 处理被中点分割的三角形
        double highSegement = 0.0;
        double lowSegement = 0.0;
        double area1 = 0.0, area2 = 0.0;
        for (const auto &triangle : earingTriList)
        {
            divideTriangle(triangle._path, triangle._area, halfPos, area1, area2);
            highSegement += area1;
            lowSegement += area2;
        }

        // 计算加权面积差
        auto weightAreas_up = (highAreas + highSegement) * ratio;
        auto weightAreas_down = (lowAreas + lowSegement) * r2;
        // 更新结果
        result.updateResult(halfPos, weightAreas_up - weightAreas_down);
        // 误差满足要求则返回
        if (_d->_maxError > result._error) return;

        // 根据面积差递归搜索
        if (weightAreas_up > weightAreas_down) getWeightPos(ratio, halfPos, pos2, result);
        getWeightPos(ratio, pos1, halfPos, result);
    }
}




///
/// @brief 计算水平分割线与边的交点
/// @param p1 边的第一个端点
/// @param p2 边的第二个端点
/// @param pos 水平分割线的Y坐标
/// @param pt 输出参数,计算得到的交点坐标
/// @details 实现步骤:
///   1. 根据水平分割线位置pos计算交点的X坐标
///   2. Y坐标直接使用分割线位置pos
///   3. 使用线性插值计算交点位置
///
void PolygonsDivider::calcInsertPt(const IntPoint &p1, const IntPoint &p2,
                                   const long long &pos, DoublePoint &pt)
{
    // 使用线性插值计算交点X坐标
    pt.X = (double)p1.X + (p2.X - p1.X) * (pos - p1.Y) / double(p2.Y - p1.Y);
    // Y坐标为水平分割线位置
    pt.Y = pos;
}


///
/// @brief 计算并验证分割后两个路径的面积
/// @param totalArea 原始多边形总面积
/// @param path_up 上部分路径
/// @param path_down 下部分路径
/// @param area_up 输出参数,上部分面积
/// @param area_down 输出参数,下部分面积
/// @details 实现步骤:
///   1. 计算上部分路径面积
///   2. 计算下部分路径面积
///   3. 验证分割前后面积是否守恒
///
inline void calcPathArea(const double &totalArea, const DoublePath &path_up, const DoublePath &path_down,
                         double &area_up, double &area_down)
{
    // 计算上部分面积
    area_up = Area(path_up);
    // 计算下部分面积
    area_down = Area(path_down);
    
    // 验证分割前后面积是否守恒(误差阈值1E-2)
    if (fabs(totalArea - area_up - area_down) > 1E-2 )
    {
        qDebug() << "[Error] calcPathArea" << totalArea << area_up << area_down << path_up << path_down;
    }
}

///
/// @brief 使用水平线分割三角形
/// @param triangle 输入三角形路径
/// @param totalArea 三角形总面积
/// @param pos 水平分割线Y坐标
/// @param area1 输出参数,分割后上部分面积
/// @param area2 输出参数,分割后下部分面积
/// @details 实现步骤:
///   1. 获取三角形三个顶点
///   2. 根据顶点与分割线的位置关系分类讨论
///   3. 计算分割线与三角形边的交点
///   4. 构造分割后的上下两个多边形
///   5. 计算并验证分割后的面积
///
void PolygonsDivider::divideTriangle(const Path &triangle, const double &totalArea,
                                     const long long &pos, double &area1, double &area2)
{
    // 验证输入三角形顶点数
    if (3 != triangle.size()) return;

    // 存储分割后上下两部分的顶点
    DoublePath upperPtVec, downnerPtVec;
    const auto p1 = triangle.cbegin();
    auto p2 = std::next(p1);
    auto p3 = std::next(p2);

    // 用于存储交点坐标
    DoublePoint pt;

    // 根据第一个顶点与分割线的位置关系分三种情况处理
    if (p1->Y > pos)
    {
        upperPtVec.push_back(*p1);
        if (p2->Y >= pos)
        {
            // p1在分割线上方的情况
            upperPtVec.push_back(*p2);
            if (p3->Y >= pos)
            {
                area1 = totalArea;
                area2 = 0;
                return;
            }
            else
            {
                if (pos == p2->Y)
                {
                    downnerPtVec.push_back(*p2);
                    downnerPtVec.push_back(*p3);
                }
                else
                {
                    calcInsertPt(*p2, *p3, pos, pt);
                    upperPtVec.push_back(pt);

                    downnerPtVec.push_back(pt);
                    downnerPtVec.push_back(*p3);
                }

                calcInsertPt(*p3, *p1, pos, pt);
                upperPtVec.push_back(pt);
                downnerPtVec.push_back(pt);

                calcPathArea(totalArea, upperPtVec, downnerPtVec, area1, area2);
                return;
            }
        }
        else
        {
            {
                calcInsertPt(*p1, *p2, pos, pt);
                upperPtVec.push_back(pt);

                downnerPtVec.push_back(pt);
                downnerPtVec.push_back(*p2);
            }

            if (pos == p3->Y)
            {
                upperPtVec.push_back(*p3);
                downnerPtVec.push_back(*p3);
            }
            else if (p3->Y < pos)
            {
                calcInsertPt(*p1, *p3, pos, pt);
                upperPtVec.push_back(pt);

                downnerPtVec.push_back(*p3);
                downnerPtVec.push_back(pt);
            }
            else
            {
                calcInsertPt(*p2, *p3, pos, pt);
                upperPtVec.push_back(pt);
                upperPtVec.push_back(*p3);

                downnerPtVec.push_back(pt);
            }
            calcPathArea(totalArea, upperPtVec, downnerPtVec, area1, area2);
            return;
        }
    }
    else if (p1->Y == pos)
    {
        // p1在分割线上的情况
        if (p2->Y > pos)
        {
            upperPtVec.push_back(*p1);
            upperPtVec.push_back(*p2);
            if (p3->Y >= pos)
            {
                area1 = totalArea;
                area2 = 0;
                return;
            }
            else
            {
                calcInsertPt(*p2, *p3, pos, pt);
                upperPtVec.push_back(pt);

                downnerPtVec.push_back(pt);
                downnerPtVec.push_back(*p3);
                downnerPtVec.push_back(*p1);
            }
            calcPathArea(totalArea, upperPtVec, downnerPtVec, area1, area2);
            return;
        }
        else if (p2->Y == pos)
        {
            if (p3->Y > 0)
            {
                area1 = totalArea;
                area2 = 0;
                return;
            }
            else
            {
                area1 = 0;
                area2 = totalArea;
                return;
            }

        }
        else
        {
            downnerPtVec.push_back(*p1);
            downnerPtVec.push_back(*p2);
            if (p3->Y <= pos)
            {
                area1 = 0;
                area2 = totalArea;
                return;
            }
            else
            {
                calcInsertPt(*p2, *p3, pos, pt);
                upperPtVec.push_back(pt);
                upperPtVec.push_back(*p3);
                upperPtVec.push_back(*p1);

                downnerPtVec.push_back(pt);
            }
            calcPathArea(totalArea, upperPtVec, downnerPtVec, area1, area2);
            return;
        }
    }
    else
    {
        // p1在分割线下方的情况
        downnerPtVec.push_back(*p1);
        if (p2->Y <= pos)
        {
            downnerPtVec.push_back(*p2);
            if (p3->Y <= pos)
            {
                area1 = totalArea;
                area2 = 0;
                return;
            }
            else
            {
                if (pos == p2->Y)
                {
                    upperPtVec.push_back(*p2);
                    upperPtVec.push_back(*p3);
                }
                else
                {
                    calcInsertPt(*p2, *p3, pos, pt);
                    upperPtVec.push_back(pt);
                    upperPtVec.push_back(*p3);

                    downnerPtVec.push_back(pt);
                }

                calcInsertPt(*p3, *p1, pos, pt);
                upperPtVec.push_back(pt);

                downnerPtVec.push_back(pt);
                calcPathArea(totalArea, upperPtVec, downnerPtVec, area1, area2);
                return;
            }
        }
        else
        {
            {
                calcInsertPt(*p1, *p2, pos, pt);
                upperPtVec.push_back(pt);
                upperPtVec.push_back(*p2);

                downnerPtVec.push_back(pt);
            }

            if (pos == p3->Y)
            {
                upperPtVec.push_back(*p3);
                downnerPtVec.push_back(*p3);
            }
            else if (p3->Y > pos)
            {
                calcInsertPt(*p1, *p3, pos, pt);
                upperPtVec.push_back(*p3);
                upperPtVec.push_back(pt);

                downnerPtVec.push_back(pt);
            }
            else
            {
                calcInsertPt(*p2, *p3, pos, pt);
                upperPtVec.push_back(pt);

                downnerPtVec.push_back(pt);
                downnerPtVec.push_back(*p3);
            }
            calcPathArea(totalArea, upperPtVec, downnerPtVec, area1, area2);
            return;
        }
    }
}

//Vertical
///
/// @brief 使用二分法计算最优垂直分割位置以满足面积比例要求
/// @param ratio 目标面积比例
/// @param pos1 搜索范围起始X坐标
/// @param pos2 搜索范围结束X坐标
/// @param result 输出参数,记录最优分割结果
/// @details 实现步骤:
///   1. 检查搜索范围是否足够大
///   2. 计算中点位置
///   3. 统计完全在中点左右的三角形面积
///   4. 处理被中点分割的三角形
///   5. 计算加权面积差并更新结果
///   6. 根据面积差递归搜索更优位置
///
void PolygonsDivider::getWeightPos_ver(const double &ratio, const long long &pos1, const long long &pos2,
                                       MinErrResult &result)
{
    // 搜索范围过小则返回
    if (pos2 - pos1 < 1000) return;

    // 存储被分割的三角形
    std::list<EaringTriangle> earingTriList;
    // 计算中点位置
    auto halfPos = pos1 + ((pos2 - pos1) >> 1);

    // 计算互补比例
    double r2 = 1 - ratio;
    // 记录左右部分面积
    double highAreas = 0.0;
    double lowAreas = 0.0;

    // 统计完全在中点左右的三角形面积
    for (const auto &it : _d->_earingTriList) {
        if (halfPos <= it._minX) highAreas += it._area;
        else if (halfPos >= it._maxX) lowAreas += it._area;
        else earingTriList.push_back(it);
    }

    {
        // 处理被中点分割的三角形
        double highSegement = 0.0;
        double lowSegement = 0.0;
        double area1 = 0.0, area2 = 0.0;
        for (const auto &triangle : earingTriList)
        {
            divideTriangle_ver(triangle._path, triangle._area, halfPos, area1, area2);
            highSegement += area1;
            lowSegement += area2;
        }

        // 计算加权面积差
        auto weightAreas_up = (highAreas + highSegement) * ratio;
        auto weightAreas_down = (lowAreas + lowSegement) * r2;
        // 更新结果
        result.updateResult(halfPos, weightAreas_up - weightAreas_down);
        // 误差满足要求则返回
        if (_d->_maxError > result._error) return;

        // 根据面积差递归搜索
        if (weightAreas_up > weightAreas_down) getWeightPos_ver(ratio, halfPos, pos2, result);
        getWeightPos_ver(ratio, pos1, halfPos, result);
    }
}


///
/// @brief 计算垂直分割线与边的交点
/// @param p1 边的第一个端点
/// @param p2 边的第二个端点
/// @param pos 垂直分割线的X坐标
/// @param pt 输出参数,计算得到的交点坐标
/// @details 实现步骤:
///   1. X坐标直接使用分割线位置pos
///   2. 根据垂直分割线位置pos计算交点的Y坐标
///   3. 使用线性插值计算交点位置
///
void PolygonsDivider::calcInsertPt_ver(const IntPoint &p1, const IntPoint &p2,
                                       const long long &pos, DoublePoint &pt)
{
    // X坐标为垂直分割线位置
    pt.X = pos;
    // 使用线性插值计算交点Y坐标
    pt.Y = (double)p1.Y + (p2.Y - p1.Y) * (pos - p1.X) / double(p2.X - p1.X);
}

///
/// @brief 使用垂直线分割三角形
/// @param triangle 输入三角形路径
/// @param totalArea 三角形总面积
/// @param pos 垂直分割线X坐标
/// @param area1 输出参数,分割后右部分面积
/// @param area2 输出参数,分割后左部分面积
/// @details 实现步骤:
///   1. 获取三角形三个顶点
///   2. 根据顶点与分割线的位置关系分类讨论
///   3. 计算分割线与三角形边的交点
///   4. 构造分割后的左右两个多边形
///   5. 计算并验证分割后的面积
///
void PolygonsDivider::divideTriangle_ver(const Path &triangle, const double &totalArea,
                                         const long long &pos, double &area1, double &area2)
{
    // 验证输入三角形顶点数
    if (3 != triangle.size()) return;

    // 存储分割后左右两部分的顶点
    DoublePath leftPtVec, rightPtVec;
    const auto p1 = triangle.cbegin();
    auto p2 = std::next(p1);
    auto p3 = std::next(p2);

    // 用于存储交点坐标
    DoublePoint pt;

    // 根据第一个顶点与分割线的位置关系分三种情况处理
    if (p1->X > pos)
    {
        // p1在分割线右方的情况
        rightPtVec.push_back(*p1);
        if (p2->X >= pos)
        {
            rightPtVec.push_back(*p2);
            if (p3->X >= pos)
            {
                area1 = totalArea;
                area2 = 0;
                return;
            }
            else
            {
                if (pos == p2->X)
                {
                    leftPtVec.push_back(*p2);
                    leftPtVec.push_back(*p3);
                }
                else
                {
                    calcInsertPt_ver(*p2, *p3, pos, pt);
                    rightPtVec.push_back(pt);

                    leftPtVec.push_back(pt);
                    leftPtVec.push_back(*p3);
                }

                calcInsertPt_ver(*p3, *p1, pos, pt);
                rightPtVec.push_back(pt);
                leftPtVec.push_back(pt);

                calcPathArea(totalArea, rightPtVec, leftPtVec, area1, area2);
                return;
            }
        }
        else
        {
            {
                calcInsertPt_ver(*p1, *p2, pos, pt);
                rightPtVec.push_back(pt);

                leftPtVec.push_back(pt);
                leftPtVec.push_back(*p2);
            }

            if (pos == p3->X)
            {
                rightPtVec.push_back(*p3);
                leftPtVec.push_back(*p3);
            }
            else if (p3->X < pos)
            {
                calcInsertPt_ver(*p1, *p3, pos, pt);
                rightPtVec.push_back(pt);

                leftPtVec.push_back(*p3);
                leftPtVec.push_back(pt);
            }
            else
            {
                calcInsertPt_ver(*p2, *p3, pos, pt);
                rightPtVec.push_back(pt);
                rightPtVec.push_back(*p3);

                leftPtVec.push_back(pt);
            }
            calcPathArea(totalArea, rightPtVec, leftPtVec, area1, area2);
            return;
        }
    }
    else if (p1->X == pos)
    {
        // p1在分割线上的情况
        if (p2->X > pos)
        {
            rightPtVec.push_back(*p1);
            rightPtVec.push_back(*p2);
            if (p3->X >= pos)
            {
                area1 = totalArea;
                area2 = 0;
                return;
            }
            else
            {
                calcInsertPt_ver(*p2, *p3, pos, pt);
                rightPtVec.push_back(pt);

                leftPtVec.push_back(pt);
                leftPtVec.push_back(*p3);
                leftPtVec.push_back(*p1);
            }
            calcPathArea(totalArea, rightPtVec, leftPtVec, area1, area2);
            return;
        }
        else if (p2->X == pos)
        {
            if (p3->X > 0)
            {
                area1 = totalArea;
                area2 = 0;
                return;
            }
            else
            {
                area1 = 0;
                area2 = totalArea;
                return;
            }

        }
        else
        {
            leftPtVec.push_back(*p1);
            leftPtVec.push_back(*p2);
            if (p3->X <= pos)
            {
                area1 = 0;
                area2 = totalArea;
                return;
            }
            else
            {
                calcInsertPt_ver(*p2, *p3, pos, pt);
                rightPtVec.push_back(pt);
                rightPtVec.push_back(*p3);
                rightPtVec.push_back(*p1);

                leftPtVec.push_back(pt);
            }
            calcPathArea(totalArea, rightPtVec, leftPtVec, area1, area2);
            return;
        }
    }
    else
    {
        // p1在分割线左方的情况
        leftPtVec.push_back(*p1);
        if (p2->X <= pos)
        {
            leftPtVec.push_back(*p2);
            if (p3->X <= pos)
            {
                area1 = totalArea;
                area2 = 0;
                return;
            }
            else
            {
                if (pos == p2->X)
                {
                    rightPtVec.push_back(*p2);
                    rightPtVec.push_back(*p3);
                }
                else
                {
                    calcInsertPt_ver(*p2, *p3, pos, pt);
                    rightPtVec.push_back(pt);
                    rightPtVec.push_back(*p3);

                    leftPtVec.push_back(pt);
                }

                calcInsertPt_ver(*p3, *p1, pos, pt);
                rightPtVec.push_back(pt);

                leftPtVec.push_back(pt);
                calcPathArea(totalArea, rightPtVec, leftPtVec, area1, area2);
                return;
            }
        }
        else
        {
            {
                calcInsertPt_ver(*p1, *p2, pos, pt);
                rightPtVec.push_back(pt);
                rightPtVec.push_back(*p2);

                leftPtVec.push_back(pt);
            }

            if (pos == p3->X)
            {
                rightPtVec.push_back(*p3);
                leftPtVec.push_back(*p3);
            }
            else if (p3->X > pos)
            {
                calcInsertPt_ver(*p1, *p3, pos, pt);
                rightPtVec.push_back(*p3);
                rightPtVec.push_back(pt);

                leftPtVec.push_back(pt);
            }
            else
            {
                calcInsertPt_ver(*p2, *p3, pos, pt);
                rightPtVec.push_back(pt);

                leftPtVec.push_back(pt);
                leftPtVec.push_back(*p3);
            }
            calcPathArea(totalArea, rightPtVec, leftPtVec, area1, area2);
            return;
        }
    }
}

// double calcPathsWeight(const ClipperLib::Paths &paths)
// {
//     double weight_jump = 0;
//     double weight_mark = 0;
//     bool firstAction = true;
//     long long lastX = 0;
//     long long lastY = 0;

//     for (const auto &path : paths)
//     {
//         if (path.size() < 2) continue;

//         auto it = path.cbegin();
//         if (firstAction)
//         {
//             firstAction = false;
//             lastX = it->X;
//             lastY = it->Y;
//             continue;
//         }
//         else
//         {
//             weight_jump += sqrt(pow(it->X - lastX, 2) + pow(it->Y - lastY, 2));
//             lastX = it->X;
//             lastY = it->Y;
//         }
//         do
//         {
//             ++ it;
//             if (it == path.cend()) break;
//             weight_mark += sqrt(pow(it->X - lastX, 2) + pow(it->Y - lastY, 2));
//             lastX = it->X;
//             lastY = it->Y;

//         }
//         while(true);
//     }
//     qDebug() << "weight_jump" << weight_jump << weight_mark;
//     return weight_jump;
// }
