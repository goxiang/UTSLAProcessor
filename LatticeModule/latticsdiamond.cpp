#include "latticsdiamond.h"
#include "publicheader.h"
#include "meshbase.h"

#include "clipper2/clipper.h"

#include <QVector3D>

using namespace ClipperLib;
using namespace meshdef;

///
/// @brief 根据圆柱坐标生成三维顶点
/// @param r 半径
/// @param z 高度
/// @param angle 角度(度)
/// @return 返回三维顶点坐标
/// @details 实现步骤:
///   1. 将角度转换为弧度
///   2. 使用三角函数计算x,y坐标
///   3. 返回三维点坐标
///
QVector3D getVertex(const float &r, const float &z, const float &angle)
{
    // 角度转弧度
    auto rad = angle * DEF_PI / 180.0f;
    // 返回三维点坐标
    return QVector3D(r * cos(rad), r * sin(rad), z);
}


/// LatticsDiamond::Priv
///
/// @brief 菱形晶格私有数据结构
/// @details 包含:
///   1. 平台参数
///   2. 晶格参数
///   3. 晶格数据容器
///   4. 单元体模型
///
struct LatticsDiamond::Priv
{
    // 平台参数
    int _platWidth = 0;        // 平台宽度
    int _platHeight = 0;       // 平台高度
    float _spacing = 10.0f;    // 晶格间距
    float _toolCompensation = 0.07f;  // 工具补偿
    int _typeNum = 1;          // 类型数量

    // 晶格数据
    Paths _latticsBorders;     // 晶格边界
    LatticsInfo _curLattics;   // 当前晶格信息
    QSet<LatticsPtr> _latticsSet;  // 晶格集合
    QList<LatticsPtr> _usedLatticsList;  // 已使用晶格列表

    // 单元体模型
    Part _cellLeg[Leg_Count] = {};  // 支腿模型数组
};


/// LatticsDiamond
LatticsDiamond::LatticsDiamond() : d(new Priv) { }

LatticsDiamond::~LatticsDiamond() { }

///
/// @brief 对二维路径进行平移变换
/// @param path 输入路径
/// @param outPath 输出路径
/// @param x x方向平移量
/// @param y y方向平移量
/// @details 实现步骤:
///   1. 创建临时路径
///   2. 遍历输入路径的每个点
///   3. 对每个点进行平移变换
///   4. 移动结果到输出路径
///
inline void transformDoublePath(const DoublePath &path, DoublePath &outPath, 
                              const float &x = 0.0f, const float &y = 0.0f)
{
    // 创建临时路径存储结果
    DoublePath tempPath;
    // 遍历输入路径的每个点
    for (const auto &pt : path)
    {
        // 对点进行平移变换
        tempPath << DoublePoint(pt.X + x, pt.Y + y);
    }
    // 移动结果到输出路径
    outPath = std::move(tempPath);
}

///
/// @brief 对二维路径进行旋转变换
/// @param path 输入路径
/// @return 返回旋转后的路径
/// @details 实现步骤:
///   1. 创建临时路径
///   2. 遍历输入路径点
///   3. 交换每个点的X,Y坐标
///   4. 返回旋转结果
///
inline DoublePath rotatePath(const DoublePath &path)
{
    // 创建临时路径
    DoublePath tempPath;
    // 遍历输入路径的每个点
    for (const auto &pt : path)
    {
        // 交换X,Y坐标实现旋转
        tempPath << DoublePoint(pt.Y, pt.X);
    }
    // 返回旋转后的路径
    return tempPath;
}

///
/// @brief 初始化菱形晶格参数和单元体模型
/// @param jsonParsing JSON配置解析器
/// @details 实现步骤:
///   1. 读取平台和晶格参数
///   2. 计算几何参数
///   3. 构建单元体顶点和面片
///   4. 创建并变换12个支腿模型
///
void LatticsDiamond::initLatticeParas(const QJsonParsing *jsonParsing)
{
    // 读取配置参数
    d->_platWidth = jsonParsing->getValue<int>("Platform/nWidth", 600) * UNITSPRECISION;
    d->_platHeight = jsonParsing->getValue<int>("Platform/nHeight", 600) * UNITSPRECISION;
    auto legDiameter = jsonParsing->getValue<float>("Lattice/legDiameter", 2);
    auto legLength = jsonParsing->getValue<float>("Lattice/legLength", 5);
    d->_toolCompensation = jsonParsing->getValue<float>("Lattice/toolCompensation", 0.07);

    // 计算几何参数
    float b = float(legDiameter * UNITSPRECISION);
    float l = float(legLength * UNITSPRECISION);
    float r = b / sqrt(3);
    float deltaZ = b * sqrt(2) * 0.25f;
    d->_spacing = (l + b / sqrt(6)) / sqrt(3);
    float deltaL = 0.0f; //b * sqrt(6) / 12;

    // 构建单元体顶点
    QVector3D p1 = getVertex(r, - deltaL, 0.0f);
    QVector3D p2 = getVertex(r, - deltaL, 120.0f);
    QVector3D p3 = getVertex(r, - deltaL, 240.0f);
    QVector3D p4 = getVertex(r, l + deltaL, -60.0f);
    QVector3D p5 = getVertex(r, l + deltaL, 60.0f);
    QVector3D p6 = getVertex(r, l + deltaL, 180.0f);

    // 定义顶点和面片数据
    const QVector<QVector3D> vertexs = {p1, p2, p3, p4, p5, p6};
    const QVector<QVector<int>> faces = {{0, 4, 3},
                                         {0, 1, 4},
                                         {1, 5, 4},
                                         {1, 2, 5},
                                         {2, 3, 5},
                                         {2, 0, 3},
                                         {3, 4, 5},
                                         {0, 2, 1}};

    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_0_0];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(135, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(d->_spacing, - d->_spacing, - d->_spacing + deltaZ);
        part.tranformPart(mat);
    }
    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_0_1];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(- 45, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(- d->_spacing, d->_spacing, - d->_spacing + deltaZ);
        part.tranformPart(mat);
    }
    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_1_0];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(225, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(0.0f, 0.0f, deltaZ);
        part.tranformPart(mat);
    }
    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_1_1];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(45, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(0.0f, 0.0f, deltaZ);
        part.tranformPart(mat);
    }

    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_2_0];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(135, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(- d->_spacing, - d->_spacing, d->_spacing + deltaZ);
        part.tranformPart(mat);
    }
    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_2_1];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(- 45, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(- d->_spacing, - d->_spacing, d->_spacing + deltaZ);
        part.tranformPart(mat);
    }

    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_3_0];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(225, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(0.0f, 2 * d->_spacing, 2 * d->_spacing + deltaZ);
        part.tranformPart(mat);
    }
    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_3_1];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(45, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(- 2 * d->_spacing, 0.0f, 2 * d->_spacing + deltaZ);
        part.tranformPart(mat);
    }

    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_4_0];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(135, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(d->_spacing, - d->_spacing, 3 * d->_spacing + deltaZ);
        part.tranformPart(mat);
    }
    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_4_1];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(- 45, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(- d->_spacing, d->_spacing, 3 * d->_spacing + deltaZ);
        part.tranformPart(mat);
    }

    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_5_0];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(225, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(0.0f, 0.0f, 4 * d->_spacing + deltaZ);
        part.tranformPart(mat);
    }
    {
        meshdef::Part &part = d->_cellLeg[LayerLeg_5_1];
        part.createPartInfo(vertexs, faces);
        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(45, QVector3D(0.0f, 0.0f, 1.0f));
        mat.rotate(54.73561f, QVector3D(0.0f, 1.0f, 0.0f));
        mat.translate(0.5f * r, 0.0f, 0.0f);
        part.tranformPart(mat);

        mat.setToIdentity();
        mat.translate(0.0f, 0.0f, 4 * d->_spacing + deltaZ);
        part.tranformPart(mat);
    }
}

///
/// @brief 创建指定高度的晶格切片轮廓
/// @param layerHei 切片高度
/// @return 返回切片轮廓路径集合
/// @details 实现步骤:
///   1. 计算单元高度
///   2. 切片所有支腿
///   3. 移动并合并轮廓
///
DoublePaths LatticsDiamond::createLattics(const int &layerHei) 
{
    // 创建路径集合
    DoublePaths paths;

    // 计算周期性单元高度
    float cellHei = layerHei;
    while (cellHei > 4 * d->_spacing) cellHei -= 4 * d->_spacing;

    // 切片12个支腿
    for (int i = 0; i < 12; ++ i)
    {
        // 获取支腿切片轮廓
        auto path = d->_cellLeg[i].cutPartOnZ(cellHei);
        // 添加有效轮廓
        if (path.size() > 2) paths << std::move(path);
    }

    // 移动并合并轮廓
    moveDoublePath(paths);
    return paths;
}


///
/// @brief 识别并分类晶格轮廓
/// @param paths 输入/输出路径集合
/// @details 实现步骤:
///   1. 清空边界和使用列表
///   2. 遍历输入路径
///   3. 识别晶格轮廓
///   4. 分离非晶格轮廓
///
void LatticsDiamond::identifyLattices(Paths &paths)
{
    // 清空数据
    d->_latticsBorders.clear();
    d->_usedLatticsList.clear();

    // 存储非晶格路径
    Paths totalPaths;
    
    // 遍历所有路径
    for (const auto &path : paths) {
        // 创建晶格信息
        auto latticsPtr = LatticsPtr(new LatticsInfo(path));
        // 检查是否为已知晶格
        if (d->_latticsSet.contains(latticsPtr)) {
            // 添加到使用列表
            d->_usedLatticsList << latticsPtr;
        }
        else {
            // 添加到非晶格路径
            totalPaths.push_back(path);
        }
    }
    
    // 更新路径集合
    paths = std::move(totalPaths);
}

///
/// @brief 获取晶格间距
/// @return 返回晶格间距值
/// @details 返回私有成员变量_spacing的值,表示晶格单元之间的距离
///
const float LatticsDiamond::getSpacing() const
{
    return d->_spacing;
}


/// Private
///
/// @brief 移动并合并双精度路径
/// @param paths 输入/输出路径集合
/// @details 实现步骤:
///   1. 计算每个路径的边界框和中心点
///   2. 基于参考中心调整路径位置
///   3. 执行路径膨胀和布尔运算
///   4. 应用工具补偿
///
void LatticsDiamond::moveDoublePath(DoublePaths &paths)
{
    // 计算双倍间距
    double dulSpacing = 2 * d->_spacing;

    // 路径信息结构体
    struct PathInfo {
        DoublePath *_path;      // 路径指针
        BOUNDINGRECT _rc;       // 边界矩形
        QPointF _center;        // 中心点

        PathInfo() = default;
        PathInfo(DoublePath &path) {
            _path = &path;
            calcBBox(path, _rc);
            _center = getCenter(_rc);
        }
    };

    // 存储路径信息
    QVector<PathInfo> pathInfoVec;

    // 处理多个路径的情况
    auto pathSz = paths.size();
    if (pathSz > 1)
    {
        // 计算所有路径信息
        for (auto &path : paths)
        {
            pathInfoVec << PathInfo(path);
        }

        // 以第一个路径为参考
        auto refCenter = pathInfoVec[0]._center;
        // 调整其他路径位置
        for (int i = 1; i < pathSz; ++ i)
        {
            auto tempCenter = pathInfoVec[i]._center;
            auto dist = getDistance(refCenter, tempCenter);

            // 计算偏移方向
            auto offsetX = (tempCenter.x() > refCenter.x()) ? -dulSpacing : dulSpacing;
            auto offsetY = (tempCenter.y() > refCenter.y()) ? -dulSpacing : dulSpacing;
            auto tempDist = getDistance(refCenter, tempCenter, offsetX, offsetY);

            // 如果偏移后距离更近则应用偏移
            if (tempDist < dist) transformDoublePath(paths[i], paths[i], offsetX, offsetY);
        }

        // 执行路径处理
        auto pathD = *reinterpret_cast<Clipper2Lib::PathsD *>(&paths);
        // 路径膨胀
        pathD = Clipper2Lib::InflatePaths(pathD, 100, Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon);
        // 执行布尔合并
        auto tempPaths = Clipper2Lib::Union(pathD, Clipper2Lib::FillRule::NonZero);

        // 处理结果路径
        Clipper2Lib::PathsD newPaths;
        for (const auto &path : tempPaths)
        {
            // 保留外轮廓
            if (Clipper2Lib::Area(path) > 0) newPaths.push_back(path);
        }
        // 应用工具补偿
        newPaths = Clipper2Lib::InflatePaths(newPaths, -100 - d->_toolCompensation, Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon);
        paths = *reinterpret_cast<DoublePaths *>(&newPaths);
    }
}
