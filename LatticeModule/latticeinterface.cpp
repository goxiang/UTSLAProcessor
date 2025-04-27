#include "latticeinterface.h"
#include "latticsdiamond.h"
#include "./ScanLinesSortor/scanlinessortor.h"
#include "meshinfo.h"

#include <QVector3D>
#include <QMutex>
#include <QHash>

using namespace ClipperLib;

/// Priv

struct LatticeInterface::Priv {
    QSharedPointer<LatticsAbstract> _latticsInf = nullptr;
    double _shellThickness = 600.0;
    Priv() {
        _latticsInf = QSharedPointer<LatticsAbstract>(new LatticsDiamond);
    }
    ///
    /// @brief 计算路径的腔体结构
    /// @param paths 输入路径集合
    /// @param cavity 输出腔体路径
    /// @details 实现步骤:
    ///   1. 构建多边形信息树结构
    ///   2. 按面积排序并建立父子关系
    ///   3. 分层处理多边形嵌套
    ///   4. 根据面积比和偏移计算腔体
    ///   5. 简化最终结果
    ///
    void calcuCavity(const Paths &paths, Paths &cavity) {
        // 多边形信息结构体定义
        struct PolyInfo {
            int _index = 0;            // 多边形索引
            int _maxDepth = 0;         // 最大深度
            double _area = 0.0;        // 面积
            bool _isOutter = false;    // 是否外轮廓
            BoundingBox _bBox;         // 边界框
            PolyInfo *_parent = nullptr;  // 父节点
            QVector<PolyInfo *> _children;  // 子节点列表

            // 用于管理多边形的树形结构和属性

            /// @brief 构造函数,设置多边形索引
            /// @param index 多边形索引值
            PolyInfo(const int &index) { _index = index; }

            /// @brief 设置多边形面积和类型
            /// @param area 面积值
            /// @details 根据面积正负判断是否为外轮廓
            void setArea(const double &area) {
                _area = area;
                _isOutter = bool(_area > 0);
            }

            /// @brief 设置父节点关系
            /// @param poly 父节点指针
            /// @details 更新父子关系和最大深度
            void setParent(PolyInfo *poly) {
                this->_parent = poly;
                poly->_children << this;
                if (poly->_maxDepth < _maxDepth + 1) poly->_maxDepth = _maxDepth + 1;
            }

            /// @brief 获取指定深度的节点集合
            /// @param depth 目标深度
            /// @return 返回该深度的所有节点
            /// @details 递归获取子节点
            const QVector<PolyInfo *> getDeepNodes(int depth) {
                // 深度超出范围返回空
                if (depth > _maxDepth) return QVector<PolyInfo *>();
                
                // 深度为0返回直接子节点
                if (0 == depth) return _children;
                // 无子节点返回空
                if (_children.size() < 1) return QVector<PolyInfo *>();
                // 递归获取子节点
                QVector<PolyInfo *> targetVec;
                for (const auto &node : qAsConst(_children)) {
                    targetVec << node->getDeepNodes(depth -1);
                }
                return targetVec;
            }

            /// @brief 计算所有子节点面积和
            /// @return 返回子节点面积绝对值之和
            double getChildernArea() {
                auto area = 0.0;
                for (const auto &node : qAsConst(_children)) {
                    area += fabs(node->_area);
                }
                return area;
            }
        };

        // 创建多边形信息向量
        QVector<PolyInfo *> polyInfoVec;
        auto index = 0;
        for (const auto &path : paths) {
            auto polyInfo = new PolyInfo(index);
            polyInfo->setArea(Area(path, polyInfo->_bBox));
            polyInfoVec << std::move(polyInfo);
            ++ index;
        }

        // 按面积大小排序
        std::sort(polyInfoVec.begin(), polyInfoVec.end(),
                  [](const PolyInfo *p1, const PolyInfo *p2) -> bool {
            return fabs(p1->_area) < fabs(p2->_area);
                  });
        // 定义查找父节点的lambda函数
        auto findParentNode = [&](const PolyInfo *p1, const PolyInfo *p2) ->bool {
            // 检查边界框包含关系和点包含关系
            if (p1->_isOutter == p2->_isOutter) return false;
            if (p1->_bBox.minX <= p2->_bBox.minX) return false;
            if (p1->_bBox.maxX >= p2->_bBox.maxX) return false;
            if (p1->_bBox.minY <= p2->_bBox.minY) return false;
            if (p1->_bBox.maxY >= p2->_bBox.maxY) return false;

            return (1 == PointInPolygon(paths[p1->_index].front(), paths[p2->_index]));
        };

        // 构建多边形树结构
        PolyInfo root(-1);
        while (polyInfoVec.size()) {
            // 为每个多边形找到合适的父节点
            bool foundPoly = false;
            int polySz = polyInfoVec.size();
            for (int j = 1; j < polySz; ++ j) {
                if (findParentNode(polyInfoVec[0], polyInfoVec[j])) {
                    foundPoly = true;
                    polyInfoVec[0]->setParent(polyInfoVec[j]);
                    break;
                }
            }
            if (false == foundPoly) polyInfoVec[0]->setParent(&root);
            polyInfoVec.takeFirst();
        }

        // 处理多边形嵌套关系生成腔体
        QVector<PolyInfo *> targetPolys;
        int step = 0;
        // 按层遍历多边形树(每次跳过2层)
        for (int iDepth = 0; iDepth < root._maxDepth; iDepth += 2) {
            // 获取当前层的所有节点
            for (const auto &node : root.getDeepNodes(iDepth)) {
                // 跳过叶子节点
                if (node->_children.size() < 1) continue;

                // 计算当前节点的偏移路径
                Paths tempPath;
                {
                    ClipperOffset o;
                    o.AddPath(paths[node->_index], jtMiter, etClosedPolygon);
                    o.Execute(tempPath, - _shellThickness);
                }
                // 检查偏移后面积是否足够大
                auto tempArea = Areas(tempPath);
                if (tempArea < 1E6) continue;

                // 计算子节点面积占比
                auto ratio = node->getChildernArea() / tempArea;
                if (ratio > 0.99) {
                    // 面积比大于99%时根据步骤选择处理方式
                    if (step & 0x1) targetPolys << node;
                    else targetPolys << node->_children;
                }
                else {
                    // 面积比较小时逐个处理子节点
                    for (const auto &subNode : qAsConst(node->_children)) {
                        // 计算子节点与偏移路径的差集
                        Paths solution;
                        Clipper c;
                        c.AddPath(paths[subNode->_index], PolyType::ptSubject, true);
                        c.AddPaths(tempPath, PolyType::ptClip, true);
                        c.Execute(ClipType::ctDifference, solution, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);

                        // 有差集结果则保留该子节点
                        if (solution.size()) targetPolys << subNode;
                    }
                }
            }
            ++ step;
        }


        // int repeatCnt = (root._maxDepth / 4) + 1;
        // auto tempIndex = 0;
        // for (int i = 0; i < repeatCnt; ++ i) {
        //     tempIndex = (i << 2);
        //     if (tempIndex + 2 == root._maxDepth)
        //     {

        //     }
        //     else if (tempIndex + 4 <= root._maxDepth) {

        //     }
        //     if (tempIndex + 2 == root._maxDepth ||
        //         tempIndex + 4 <= root._maxDepth) targetPolys << root.getDeepNodes(tempIndex + 0);
        //     if (tempIndex + 4 <= root._maxDepth) targetPolys << root.getDeepNodes(tempIndex + 3);
        // }
        // 生成最终腔体路径
        for (const auto &node : qAsConst(targetPolys)) {
            cavity << paths[node->_index];
        }
        // 简化结果路径
        if (cavity.size()) SimplifyPolygons(cavity);
    }
};

/// LatticeInterface
///
/// @brief 晶格接口类构造函数
/// @details 创建私有实现对象
///
LatticeInterface::LatticeInterface() : d(new Priv) { }

///
/// @brief 初始化晶格参数
/// @param jsonParsing JSON配置解析器
/// @details 通过晶格实现对象初始化参数
///
void LatticeInterface::initLatticeParas(const QJsonParsing *jsonParsing)
{
    // 检查晶格实现对象是否存在
    if (nullptr == d->_latticsInf) return;
    // 调用实现对象初始化参数
    d->_latticsInf->initLatticeParas(jsonParsing);
}

///
/// @brief 计算层晶格结构
/// @param paths 输入/输出路径集合
/// @param refPaths 参考路径集合
/// @param layer 层号
/// @param lattices 输出晶格路径
/// @param latticeLayerInfo 输出层晶格信息
/// @return 是否需要生成晶格
/// @details 实现步骤:
///   1. 初始化数据
///   2. 生成基础晶格单元
///   3. 在边界框内填充晶格
///   4. 执行布尔运算
///   5. 识别和分类晶格
///
bool LatticeInterface::calcLayerLattic(Paths &paths, const Paths &refPaths, const int &layer,
                                       ClipperLib::Paths &lattices, LatticeLayerInfo &latticeLayerInfo)
{
    // 初始化层信息
    latticeLayerInfo.initialize();
    if (nullptr == d->_latticsInf) return false;

    // Paths pathsArea;
    // d->calcuCavity(paths, pathsArea);

    // 处理参考路径
    Paths pathsArea = std::move(refPaths);
    bool needLattic = pathsArea.size();
    if (false == needLattic) return needLattic;
    SimplifyPolygons(pathsArea, pftEvenOdd);
    if (pathsArea.size() < 1) return false;

    // 计算边界框
    Paths latticePaths;
    BoundingBox boundingBox;
    Areas(pathsArea, boundingBox);

    // 创建基础晶格单元
    auto layerPaths = d->_latticsInf->createLattics(layer);

    // 获取晶格间距
    auto spacing = d->_latticsInf->getSpacing();
    auto moveSpacing = 4.0 * spacing;
    QSet<LatticeInfo> latticeSet;

    // 在边界框内填充晶格
    int curType = -1;
    int maxPtSz = 0;
    for (const auto &path : layerPaths)
    {
        // 计算当前路径信息
        ++ curType;
        auto area = Area(path);
        BoundingBoxD rc;
        calcBBox(path, rc);

        if (maxPtSz < path.size()) maxPtSz = path.size();

        // 计算初始位置
        auto initX = 0;
        auto initY = 0;

        while ((rc._maxX + initX) > 0)
        {
            initX -= moveSpacing;
        }
        while ((rc._maxY + initY) > 0)
        {
            initY -= moveSpacing;
        }
        // 按间距填充晶格
        int oddCnt = 0;

        while (true)
        {
            // Y方向填充
            if (rc._maxY + initY >= boundingBox.minY)
            {
                // X方向填充
                auto tempX = initX - (oddCnt ? (2 * spacing) : 0.0);
                while (true)
                {
                    // 添加晶格单元
                    if (rc._maxX + tempX >= boundingBox.minX)
                    {
                        auto tempPath = transformPath(path, tempX, initY);
                        BOUNDINGRECT tempRc;
                        calcBBox(tempPath, tempRc);
                        latticeSet.insert(LatticeInfo(curType, area, tempRc.minX, tempRc.minY));
                        latticePaths << std::move(tempPath);
                    }
                    tempX += moveSpacing;
                    if (rc._minX + tempX > boundingBox.maxX) break;
                }
            }

            oddCnt = 1 - oddCnt;
            initY += 2.0 * spacing;
            if (rc._minY + initY > boundingBox.maxY) break;
        }
    }

    // 执行布尔运算
    if (latticePaths.size() < 1) return false;
    {
        // 与参考路径求交
        Clipper c;
        c.AddPaths(pathsArea, ptSubject, true);
        c.AddPaths(latticePaths, ptClip, true);
        c.Execute(ctIntersection, latticePaths, pftEvenOdd, pftNonZero);
        if (latticePaths.size() < 1) return false;
    }
    {
        // 与输入路径求并
        Clipper c;
        c.AddPaths(paths, ptSubject, true);
        c.AddPaths(latticePaths, ptClip, true);
        c.Execute(ctUnion, latticePaths, pftEvenOdd, pftEvenOdd);
    }
    // {
    //     Clipper c;
    //     c.AddPaths(latticePaths, ptSubject, true);
    //     c.AddPaths(paths, ptClip, true);
    //     c.Execute(ctDifference, lattices, pftEvenOdd, pftEvenOdd);
    // }
    // return false;

    // 识别和分类晶格
    paths.clear();
    maxPtSz *= 1.5;
// #pragma omp parallel for num_threads(6)
// #pragma omp parallel for
//     for (int i = 0; i < latticePaths.size(); ++ i)
//     {
//         auto it = &latticePaths[i];

    for (auto it = latticePaths.begin(); it != latticePaths.end(); ++ it)
    {
        if (it->size() < maxPtSz)
        {
            // 计算当前路径信息
            BoundingBox rc;
            auto area = Area(*it, rc);
            if (area > 0)
            {
                // 查找匹配的晶格类型
                auto setIt = latticeSet.find(LatticeInfo(0, area, rc.minX, rc.minY));
                if (setIt != latticeSet.end())
                {
                    // 添加晶格信息
                    if (false == latticeLayerInfo._latticePathInfo.contains(setIt->_type))
                    {
                        LatticePathInfo latticePathInfo;
                        latticePathInfo._type = setIt->_type;
                        latticePathInfo._path = std::move(*it);
                        latticePathInfo._centerX = setIt->_centerX;
                        latticePathInfo._centerY = setIt->_centerY;
                        latticeLayerInfo._latticePathInfo.insert(setIt->_type, latticePathInfo);
                    }
                    latticeLayerInfo._latticeInfo.push_back(std::move(*setIt));
                }
                else paths << std::move(*it);
            }
            else paths << std::move(*it);
        }
        else paths << std::move(*it);
    }
    // qDebug() << "find cnt" << laticeVec.size() << "/" << latticePaths.size() << paths.size();

    // 排序晶格信息
    if (latticeLayerInfo._latticeInfo.size() > 1) ScanLinesSortor::sortDatas(latticeLayerInfo._latticeInfo);
    // qDebug() << "~~~~~~~~~~~~~~~~~~calcIndexVec" << laticeVec.size();

    return needLattic;
}
