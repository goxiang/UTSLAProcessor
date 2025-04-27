#ifndef LATTICSABSTRACT_H
#define LATTICSABSTRACT_H

#include "bpccommon.h"

enum LatticeLegType {
    LayerLeg_0_0 = 0,
    LayerLeg_0_1 = 1,
    LayerLeg_1_0 = 2,
    LayerLeg_1_1 = 3,
    LayerLeg_2_0 = 4,
    LayerLeg_2_1 = 5,
    LayerLeg_3_0 = 6,
    LayerLeg_3_1 = 7,
    LayerLeg_4_0 = 8,
    LayerLeg_4_1 = 9,
    LayerLeg_5_0 = 10,
    LayerLeg_5_1 = 11,

    Leg_Count = 12
};

class QJsonParsing;
///
/// ! @coreclass{LatticsAbstract}
/// 整个晶格处理系统的基础框架，确保不同类型晶格实现的一致性
///
class LatticsAbstract {
public:
    LatticsAbstract() = default;
    virtual ~LatticsAbstract() = default;

    virtual void initLatticeParas(const QJsonParsing *) = 0;
    virtual DoublePaths createLattics(const int &) = 0;
    virtual void identifyLattices(ClipperLib::Paths &) = 0;
    virtual const float getSpacing() const = 0;
};

struct LatticsInfo {
    int _type = 0;
    double _area = 0.0;
    IntPoint _centerPt;

    LatticsInfo() = default;
    LatticsInfo(const Path &path) {
        BoundingBox _bBox;
        _area = Area(path, _bBox);
        _centerPt.X = qRound((_bBox.minX + _bBox.maxX) * 0.5);
        _centerPt.Y = qRound((_bBox.minY + _bBox.maxY) * 0.5);
    }
    inline bool operator==(const LatticsInfo &other) const {
        return (_centerPt.X == other._centerPt.X) &&
               (_centerPt.Y == other._centerPt.Y) &&
               (fabs(_area - other._area) < 1E-6);
    }
};
typedef QSharedPointer<LatticsInfo> LatticsPtr;


struct BoundingBoxD {
    double _minX = 1E+30;
    double _maxX = 1E-30;
    double _minY = 1E+30;
    double _maxY = 1E-30;

    QPointF getCenter() {
        QPointF pt;
        pt.setX((_minX + _maxX) * 0.5);
        pt.setY((_minY + _maxY) * 0.5);
        return pt;
    }
};

///
/// @brief 计算路径的边界框
/// @param path 输入路径
/// @param bBox 输出边界框
/// @details 实现步骤:
///   1. 遍历路径上的所有点
///   2. 更新最小最大X坐标
///   3. 更新最小最大Y坐标
///   4. 得到包围整个路径的矩形框
///
inline void calcBBox(const Path &path, BOUNDINGRECT &bBox)
{
    // 遍历路径上的每个点
    for (const auto &pt : path)
    {
        // 更新X方向边界
        if (pt.X < bBox.minX) bBox.minX = pt.X;
        if (pt.X > bBox.maxX) bBox.maxX = pt.X;
        // 更新Y方向边界
        if (pt.Y < bBox.minY) bBox.minY = pt.Y; 
        if (pt.Y > bBox.maxY) bBox.maxY = pt.Y;
    }
}


inline void calcBBox(const DoublePath &path, BOUNDINGRECT &bBox)
{
    for (const auto &pt : path)
    {
        if (pt.X < bBox.minX) bBox.minX = pt.X;
        if (pt.X > bBox.maxX) bBox.maxX = pt.X;
        if (pt.Y < bBox.minY) bBox.minY = pt.Y;
        if (pt.Y > bBox.maxY) bBox.maxY = pt.Y;
    }
}

inline void calcBBox(const DoublePath &path, BoundingBoxD &bBox)
{
    for (const auto &pt : path)
    {
        if (pt.X < bBox._minX) bBox._minX = pt.X;
        if (pt.X > bBox._maxX) bBox._maxX = pt.X;
        if (pt.Y < bBox._minY) bBox._minY = pt.Y;
        if (pt.Y > bBox._maxY) bBox._maxY = pt.Y;
    }
}

///
/// @brief 将双精度路径转换为整数路径并平移
/// @param path 输入的双精度路径
/// @param x x方向平移量
/// @param y y方向平移量
/// @return 返回转换后的整数路径
/// @details 实现步骤:
///   1. 创建临时整数路径
///   2. 遍历输入路径点
///   3. 转换精度并平移坐标
///   4. 返回结果路径
///
inline Path transformPath(const DoublePath &path, const float &x, const float &y)
{
    // 创建临时路径
    Path tempPath;
    // 遍历输入路径的每个点
    for (const auto &pt : path)
    {
        // 转换为整数坐标并平移
        tempPath << IntPoint(qRound(pt.X + x), qRound(pt.Y + y));
    }
    // 返回转换后的路径
    return tempPath;
}


inline QDebug operator<<(QDebug debug, const BOUNDINGRECT &p)
{
    debug.noquote() << "{" << p.minX  << " " << p.maxX  << " " << p.minY  << " "  << p.maxY << "}";
    return debug;
}

///
/// @brief 计算边界框的中心点
/// @param rc 输入边界框
/// @return 返回中心点坐标
/// @details 实现步骤:
///   1. 计算X坐标中点
///   2. 计算Y坐标中点
///   3. 返回中心点
///
inline QPointF getCenter(BOUNDINGRECT &rc)
{
    return QPointF((rc.minX + rc.maxX) * 0.5, (rc.minY + rc.maxY) * 0.5);
}

///
/// @brief 计算两点间距离的平方
/// @param p1 第一个点
/// @param p2 第二个点
/// @return 返回距离平方值
/// @details 实现步骤:
///   1. 计算X方向差值平方
///   2. 计算Y方向差值平方
///   3. 返回平方和
///
inline double getDistance(const QPointF &p1, const QPointF &p2)
{
    return (pow(p1.x() - p2.x(), 2) + pow(p1.y() - p2.y(), 2));
}


inline double getDistance(const QPointF &p1, const QPointF &p2, const double &x, const double &y)
{
    return (pow(p1.x() - p2.x() - x, 2) + pow(p1.y() - p2.y() - y, 2));
}

#endif // LATTICSABSTRACT_H
