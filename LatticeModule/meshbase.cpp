#include "meshbase.h"

#define _LL_ADDRESS(lp) reinterpret_cast<long long>(lp)
using namespace ClipperLib;

namespace meshdef {

///
/// @brief 获取边的哈希值
/// @param edge 边指针
/// @return 返回由边的两个顶点地址构成的键值对
/// @details 按顶点顺序生成哈希值
///
inline QPair<long long, long long> getEdgeHash(Edge *edge)
{
    // 返回边的起点和终点地址构成的键值对
    return QPair<long long, long long>(_LL_ADDRESS(edge->_vertexs[0]),
                                     _LL_ADDRESS(edge->_vertexs[1]));
}

///
/// @brief 获取半边的哈希值
/// @param edge 边指针
/// @return 返回由边的两个顶点地址构成的键值对
/// @details 按相反顺序生成哈希值
///
inline QPair<long long, long long> getHalfEdgeHash(Edge *edge)
{
    // 返回边的终点和起点地址构成的键值对
    return QPair<long long, long long>(_LL_ADDRESS(edge->_vertexs[1]),
                                     _LL_ADDRESS(edge->_vertexs[0]));
}

///
/// @brief 获取边的唯一哈希值
/// @param edge 边指针
/// @return 返回由边的两个顶点地址构成的键值对
/// @details 按顶点地址大小排序生成哈希值
///
inline QPair<long long, long long> getUniqueEdgeHash(Edge *edge)
{
    // 返回边的两个顶点地址按大小排序后构成的键值对
    return QPair<long long, long long>(
        _LL_ADDRESS(std::min(edge->_vertexs[0], edge->_vertexs[1])),
        _LL_ADDRESS(std::max(edge->_vertexs[0], edge->_vertexs[1])));
}

///
/// @brief 对顶点进行变换
/// @param mat 变换矩阵
/// @details 使用矩阵对顶点坐标进行映射变换
///
void Vertex::transform(const QMatrix4x4 &mat)
{
    // 使用变换矩阵映射顶点坐标
    _coor = mat.map(_coor);
}


/// Edge
///
/// @brief 计算边与切片平面的交点
/// @param edgeResult 边的计算结果
/// @param clipResult 切片结果
/// @return 是否有交点
/// @details 实现步骤:
///   1. 计算两端点到切片面的距离
///   2. 检查端点是否已被使用
///   3. 检查端点是否在切片面上
///   4. 计算边与切片面的交点
///
bool Edge::calcIntersection(CalcEdgeResult &edgeResult, ClipPathResult &clipResult)
{
    // 获取切片高度
    auto layerHei = clipResult._layerHei;
    // 计算两端点到切片面的距离
    auto delV1 = _vertexs[0]->_coor.z() - layerHei;
    auto delV2 = layerHei - _vertexs[1]->_coor.z();

    // 检查第一个端点是否已被使用
    if (clipResult._usedVertexSet.contains(_LL_ADDRESS(_vertexs[0])))
    {
        // 如果第二个端点在切片面上且未被使用
        if (fabs(delV2) < 1E-6 && false == clipResult._usedVertexSet.contains(_LL_ADDRESS(_vertexs[1])))
        {
            clipResult._usedVertexSet.insert(_LL_ADDRESS(_vertexs[1]));
            edgeResult.setVertex(_vertexs[1]);
            return true;
        }
        goto LABEL_NO_INSECTION;
    }

    // 检查第二个端点是否已被使用
    if (clipResult._usedVertexSet.contains(_LL_ADDRESS(_vertexs[1])))
    {
        // 如果第一个端点在切片面上且未被使用
        if (fabs(delV1) < 1E-6 && false == clipResult._usedVertexSet.contains(_LL_ADDRESS(_vertexs[0])))
        {
            clipResult._usedVertexSet.insert(_LL_ADDRESS(_vertexs[0]));
            edgeResult.setVertex(_vertexs[0]);
            return true;
        }
        goto LABEL_NO_INSECTION;
    }

    // 检查端点是否在切片面上
    if (fabs(delV1) < 1E-6)
    {
        clipResult._usedVertexSet.insert(_LL_ADDRESS(_vertexs[0]));
        edgeResult.setVertex(_vertexs[0]);
        return true;
    }
    if (fabs(delV2) < 1E-6)
    {
        clipResult._usedVertexSet.insert(_LL_ADDRESS(_vertexs[1]));
        edgeResult.setVertex(_vertexs[1]);
        return true;
    }

    // 计算边与切片面的交点
    if (delV1 * delV2 > 0)
    {
        edgeResult._result = ERT_ONEDGE;
        edgeResult._vertex = nullptr;
        auto &coor = edgeResult._coor;
        // 使用线性插值计算交点坐标
        coor.setX(_vertexs[1]->_coor.x() +
                  delV2 / (delV1 + delV2) *
                      (_vertexs[0]->_coor.x() - _vertexs[1]->_coor.x()));
        coor.setY(_vertexs[1]->_coor.y() +
                  delV2 / (delV1 + delV2) *
                      (_vertexs[0]->_coor.y() - _vertexs[1]->_coor.y()));
        coor.setZ(layerHei);
        return true;
    }

LABEL_NO_INSECTION:
    edgeResult._result = ERT_NOINSECITON;
    edgeResult._vertex = nullptr;
    return false;
}


/// face

///
/// @brief 三角面构造函数,创建一个由三个顶点构成的三角面
/// @param v1 第一个顶点指针
/// @param v2 第二个顶点指针
/// @param v3 第三个顶点指针
/// @details 实现步骤:
///   1. 初始化顶点数组
///   2. 创建三条边
///   3. 更新顶点的面列表
///   4. 计算面法向量
///
Face::Face(Vertex *v1, Vertex *v2, Vertex *v3) {
    // 初始化顶点数组
    _vertexs.resize(3);
    _vertexs[0] = v1;
    _vertexs[1] = v2;
    _vertexs[2] = v3;

    // 创建三条边,每条边连接两个顶点
    _edges.resize(3);
    _edges[0] = new Edge(v1, v2, this);
    _edges[1] = new Edge(v2, v3, this);
    _edges[2] = new Edge(v3, v1, this);

    // 将当前面添加到各顶点的面列表中
    v1->_faceList << this;
    v2->_faceList << this;
    v3->_faceList << this;

    // 根据三个顶点坐标计算面法向量
    _normal = QVector3D::normal(v1->_coor, v2->_coor, v3->_coor);
}


///
/// @brief 判断三角面是否与切片平面相交
/// @param layerHei 切片高度
/// @return 是否有顶点在切片平面上
/// @details 实现步骤:
///   1. 遍历三角面的顶点
///   2. 检查顶点z坐标是否在切片高度上
///   3. 有任一顶点在切片面上返回true
///
bool Face::isFaceOnLayerHei(const float &layerHei)
{
    // 遍历所有顶点
    for (const auto &vertex : _vertexs)
    {
        // 检查顶点是否在切片平面上
        if (fabs(vertex->_coor.z() - layerHei) < 1E-6) return true;
    }
    return false;
}

///
/// @brief 更新三角面法向量
/// @details 使用三个顶点坐标重新计算法向量
///
void Face::updateNormal()
{
    // 根据三个顶点坐标计算面法向量
    _normal = QVector3D::normal(_vertexs[0]->_coor, _vertexs[1]->_coor, _vertexs[2]->_coor);
}


///
/// @brief 计算三角面片与切片平面的交点
/// @param clipResult 切片结果引用
/// @return 是否找到有效交点
/// @details 实现步骤:
///   1. 检查面片是否已处理
///   2. 遍历面片的边计算交点
///   3. 更新切片结果
///   4. 递归处理相邻面片
///
bool Face::calcIntersection(ClipPathResult &clipResult)
{
    // 检查面片是否已处理
    if (clipResult._usedFaceSet.contains(_LL_ADDRESS(this))) return false;
    clipResult._usedFaceSet << _LL_ADDRESS(this);
    // if (isFaceOnLayerHei(clipResult._layerHei)) return false;

    // 存储边的计算结果
    QVector<CalcEdgeResult> resultVec;
    // 遍历面片的所有边
    for (auto lpEdge : qAsConst(_edges))
    {
        // 获取边的唯一哈希值
        auto hash = getUniqueEdgeHash(lpEdge);
        // 跳过已处理的边
        if (clipResult._usedEdgeSet.contains(hash)) continue;
        clipResult._usedEdgeSet.insert(hash);

        // 计算边与切片平面的交点
        CalcEdgeResult edgeResult;
        if (false == lpEdge->calcIntersection(edgeResult, clipResult)) continue;
        edgeResult._edge = lpEdge;
        resultVec << (std::move(edgeResult));
    }
    // 无交点则返回
    if (resultVec.size() < 1) return false;

    // 更新切片结果
    if (updateClipResult(clipResult, resultVec))
    {
        // 如果起点在顶点上,处理共顶点的相邻面片
        if (ERT_ONVERTEX == clipResult._startCoor._result)
        {
            auto faceList = clipResult._startCoor._vertex->_faceList;
            for (auto *face : qAsConst(faceList))
            {
                if (clipResult._usedFaceSet.contains(_LL_ADDRESS(face))) continue;
                if (face->calcIntersection(clipResult)) break;
            }
        }
        // 如果起点在边上,处理共边的相邻面片
        else if (ERT_ONEDGE == clipResult._startCoor._result)
        {
            clipResult._startCoor._edge->_nextFace->calcIntersection(clipResult);
        }
    }
    return true;
}


///
/// @brief 处理三角面片(Face)与切片平面的交点，负责将三维模型转换为二维轮廓
/// @param clipResult 切片结果
/// @param edgeRstVec 边计算结果向量
/// @return 更新是否成功
/// @details 实现步骤:
///   1. 检查切片状态和边结果数量
///   2. 根据切片是否已开始分别处理:
///      - 未开始: 初始化切片起点
///      - 已开始: 添加切片路径点
///   3. 根据边结果数量(1-3个)采用不同处理策略
///
bool Face::updateClipResult(ClipPathResult &clipResult,
                          const QVector<CalcEdgeResult> &edgeRstVec)
{
    // 检查边结果数量是否合法(切片已开始且边结果数量大于1,则非法)
    if (clipResult._started && edgeRstVec.size() > 1)
    {
        qDebug() << "[Error]" << "Too Many edgeRstVec" << clipResult._started
                 << clipResult._edgeResultVec.size() << clipResult._path.size() << edgeRstVec.size();
        return false;
    }

    // 切片未开始的处理
    if (false == clipResult._started)
    {
        // 处理单个边结果
        if (1 == edgeRstVec.size())
        {
            // 初始化第一个边结果
            if (0 == clipResult._edgeResultVec.size())
            {
                auto &edgeRst = edgeRstVec[0];
                // 验证边结果类型
                if (ERT_ONVERTEX != edgeRst._result || nullptr == edgeRst._vertex)
                {
                    qDebug() << "[Error]" << "edgeRst Result Error" << edgeRst._result << edgeRst._vertex;
                    return false;
                }
                // 保存边结果和起点
                clipResult._edgeResultVec << edgeRstVec[0];
                clipResult._startCoor = edgeRstVec[0];
            }
            // 处理第二个边结果
            else if (1 == clipResult._edgeResultVec.size())
            {
                updateClipResult(clipResult, edgeRstVec[0], clipResult._edgeResultVec[0]);
            }
            else
            {
                qDebug() << "[Error]" << "Too Many Clip edgeRstVec" << clipResult._started
                         << clipResult._edgeResultVec.size() << clipResult._path.size() << edgeRstVec.size();
                return false;
            }
        }
        // 处理两个边结果
        else if (2 == edgeRstVec.size())
        {
            updateClipResult(clipResult, edgeRstVec[0], edgeRstVec[1]);
        }
        // 处理三个边结果
        else if (3 == edgeRstVec.size())
        {
            // 循环处理相邻边结果对
            for (int i = 0; i < edgeRstVec.size(); ++ i)
            {
                int nextIndex = i + 1;
                if (nextIndex > edgeRstVec.size() - 1) nextIndex = 0;
                if (updateClipResult(clipResult, edgeRstVec[i], edgeRstVec[nextIndex])) break;
            }
        }
    }
    // 切片已开始的处理
    else
    {
        // 添加单个切片点
        if (1 == edgeRstVec.size())
        {
            clipResult._path << DoublePoint(edgeRstVec[0]._coor.x(), edgeRstVec[0]._coor.y());
            clipResult._startCoor = edgeRstVec[0];
        }
        else if (2 == edgeRstVec.size())
        {
            // 预留两个边结果的处理
        }
    }

    return true;
}


///
/// @brief 根据两个交点更新切片结果
/// @param clipResult 切片结果引用
/// @param coor1 第一个交点结果
/// @param coor2 第二个交点结果
/// @return 是否成功更新切片结果
/// @details 实现步骤:
///   1. 确定交点所在的唯一面片
///   2. 分析面片顶点与切片平面的位置关系
///   3. 根据法向量确定交点连接顺序
///   4. 更新切片路径
///
bool Face::updateClipResult(ClipPathResult &clipResult, 
                          const CalcEdgeResult &coor1,
                          const CalcEdgeResult &coor2)
{
    // 查找包含两个交点的唯一面片
    Face *uniqueFace = nullptr;
    Vertex *v3 = nullptr;

    // 两个交点都在顶点上的情况
    if (ERT_ONVERTEX == coor1._result && ERT_ONVERTEX == coor2._result)
    {
        // 查找共享这两个顶点的面片
        for (auto *itFace : qAsConst(coor1._vertex->_faceList))
        {
            if (coor2._vertex->_faceList.contains(itFace))
            {
                // 找到第三个顶点
                for (const auto &vertex : qAsConst(itFace->_vertexs))
                {
                    if (vertex == coor1._vertex || vertex == coor2._vertex) continue;
                    v3 = vertex;
                    break;
                }

                if (nullptr == v3) qDebug() << "[Error]" << "Can not find the third vertex";
                if (fabs(v3->_coor.z() - clipResult._layerHei) < 1E-6) continue;

                uniqueFace = itFace;
                break;
            }
        }

        // 未找到唯一面片则移除顶点标记
        if (nullptr == uniqueFace)
        {
            clipResult._usedVertexSet.remove(_LL_ADDRESS(coor1._vertex));
            clipResult._usedVertexSet.remove(_LL_ADDRESS(coor2._vertex));
        }
    }
    // 第一个交点在顶点上的情况
    else if (ERT_ONVERTEX == coor1._result)
    {
        if (coor1._vertex->_faceList.contains(coor2._edge->_curFace))
        {
            uniqueFace = coor2._edge->_curFace;
        }
    }
    // 第二个交点在顶点上的情况
    else if (ERT_ONVERTEX == coor2._result)
    {
        if (coor2._vertex->_faceList.contains(coor1._edge->_curFace))
        {
            uniqueFace = coor1._edge->_curFace;
        }
    }
    // 两个交点都在边上的情况
    else
    {
        if (coor1._edge->_curFace == coor2._edge->_curFace)
        {
            uniqueFace = coor1._edge->_curFace;
        }
    }

    // 检查是否找到唯一面片
    if (nullptr == uniqueFace)
    {
        qDebug() << "[Error]" << "Can not find unique face";
        return false;
    }

    // 统计切片平面上下的顶点
    int upNum = 0;
    QList<Vertex *> upVertexList;
    QList<Vertex *> downVertexList;
    for (const auto &vertex : qAsConst(uniqueFace->_vertexs))
    {
        if (vertex->_coor.z() > clipResult._layerHei)
        {
            upVertexList << vertex;
            ++ upNum;
        }
        else if (coor1._vertex != vertex && coor2._vertex != vertex)
        {
            downVertexList << vertex;
        }
    }

    // 根据法向量确定交点连接顺序
    if (upNum && upNum < 2)
    {
        // 计算法向量
        auto normal = QVector3D::normal(coor1._coor, coor2._coor, upVertexList[0]->_coor);
        auto dir = uniqueFace->_normal[0] * normal[0] +
                   uniqueFace->_normal[1] * normal[1] +
                   uniqueFace->_normal[2] * normal[2];

        // 根据法向量方向添加路径点
        if (dir > 0)
        {
            clipResult._path << DoublePoint(coor1._coor.x(), coor1._coor.y())
                           << DoublePoint(coor2._coor.x(), coor2._coor.y());
            clipResult._startCoor = coor2;
        }
        else
        {
            clipResult._path << DoublePoint(coor2._coor.x(), coor2._coor.y())
                           << DoublePoint(coor1._coor.x(), coor1._coor.y());
            clipResult._startCoor = coor1;
        }
        clipResult._started = true;
        clipResult._edgeResultVec.clear();
        return true;
    }
    else
    {
        // 处理下方只有一个顶点的情况(确保切片轮廓与面片法向量保持一致的方向)
        if (1 == downVertexList.size())
        {
            // 计算法向量
            auto normal = QVector3D::normal(coor2._coor, coor1._coor, downVertexList[0]->_coor);
            // 通过点积与面片法向量比较
            auto dir = uniqueFace->_normal[0] * normal[0] +
                       uniqueFace->_normal[1] * normal[1] +
                       uniqueFace->_normal[2] * normal[2];
            // 根据法向量方向添加路径点
            if (dir > 0)
            {
                clipResult._path << DoublePoint(coor1._coor.x(), coor1._coor.y())
                               << DoublePoint(coor2._coor.x(), coor2._coor.y());
                clipResult._startCoor = coor2;
            }
            else
            {
                clipResult._path << DoublePoint(coor2._coor.x(), coor2._coor.y())
                               << DoublePoint(coor1._coor.x(), coor1._coor.y());
                clipResult._startCoor = coor1;
            }
            clipResult._started = true;
            clipResult._edgeResultVec.clear();
            return true;
        }
    }
    return false;
}


/// Part
///
/// @brief 创建零件信息
/// @param vertexs 顶点坐标向量
/// @param faces 面片顶点索引向量
/// @details 实现步骤:
///   1. 清空现有数据
///   2. 创建顶点列表
///   3. 验证并创建面片
///   4. 建立面片连接关系
///
void Part::createPartInfo(const QVector<QVector3D> &vertexs,
                         const QVector<QVector<int>> &faces)
{
    // 清空现有数据结构
    _faceList.clear();
    _edgeMap.clear();
    _vertexList.clear();

    // 创建顶点列表
    for (const auto &vertex : vertexs)
    {
        _vertexList << VertexPtr(new Vertex(vertex[0], vertex[1], vertex[2]));
    }

    // 获取顶点数量上限
    auto vertexNum = _vertexList.size() - 1;
    // 处理每个面片
    for (const auto &face : faces)
    {
        // 验证面片顶点数量
        if (3 != face.size()) continue;
        // 验证顶点索引有效性
        if (face[0] < 0 || face[1] < 0 || face[2] < 0) continue;
        if (face[0] > vertexNum || face[1] > vertexNum || face[2] > vertexNum) continue;

        // 创建面片并更新连接关系
        updateFaceInfo(_vertexList.at(face[0]).data(),
                      _vertexList.at(face[1]).data(),
                      _vertexList.at(face[2]).data());
    }
}


using namespace ClipperLib;
///
/// @brief 在指定高度对零件进行切片
/// @param layerHei 切片高度
/// @return 返回切片轮廓路径
/// @details 实现步骤:
///   1. 初始化切片结果
///   2. 设置切片高度
///   3. 查找切片交点
///   4. 构建完整轮廓
///
DoublePath Part::cutPartOnZ(const float &layerHei)
{
    // 创建切片结果对象
    ClipPathResult clipResult;
    // 设置切片高度
    clipResult._layerHei = layerHei;
    // 从任意面片开始查找交点
    findNextIntersection(nullptr, clipResult);
    // 返回构建的轮廓路径
    return std::move(clipResult._path);
}


///
/// @brief 对零件进行变换，零件摆放位置调整
/// @param mat 变换矩阵
/// @details 实现步骤:
///   1. 变换所有顶点坐标
///   2. 更新所有面片法向量
///
void Part::tranformPart(const QMatrix4x4 &mat)
{
    // 对所有顶点应用变换
    for (const auto &vertex : qAsConst(_vertexList)) 
        vertex->transform(mat);

    // 更新所有面片的法向量
    for (const auto &face : qAsConst(_faceList)) 
        face->updateNormal();
}

/// Private
///
/// @brief 更新零件的面片信息
/// @param v1 第一个顶点指针
/// @param v2 第二个顶点指针
/// @param v3 第三个顶点指针
/// @details 实现步骤:
///   1. 创建新的三角面片
///   2. 添加到面片列表
///   3. 处理边的连接关系
///   4. 更新边映射表
///
void Part::updateFaceInfo(Vertex *v1, Vertex *v2, Vertex *v3)
{
    // 创建新的三角面片并添加到面片列表
    auto *curFace = new Face(v1, v2, v3);
    _faceList << FacePtr(curFace);

    // 处理面片的每条边
    for (auto edge : qAsConst(curFace->_edges))
    {
        // 获取边的哈希值
        auto pair = getEdgeHash(edge);
        // 检查是否存在重复边
        if (_edgeMap.contains(pair))
        {
            qDebug() << "[ Error ]" << "Repeat Edge";
            return;
        }

        // 获取半边哈希值
        auto halfPair = getHalfEdgeHash(edge);
        // 如果存在对应的半边,建立相邻面片关系
        if (_edgeMap.contains(halfPair))
        {
            _edgeMap[halfPair]->_nextFace = edge->_curFace;
            edge->_nextFace = _edgeMap[halfPair]->_curFace;
        }

        // 将边添加到映射表
        _edgeMap.insert(pair, EdgePtr(edge));
    }
}


///
/// @brief 查找下一个切片交点
/// @param face 起始面片指针(可为空)
/// @param clipResult 切片结果引用
/// @details 实现步骤:
///   1. 如果没有指定面片:
///      - 遍历所有面片
///      - 找到第一个有交点的面片
///   2. 如果指定了面片:
///      - 直接计算该面片的交点
///
void Part::findNextIntersection(Face *face, ClipPathResult &clipResult)
{
    // 未指定面片时遍历所有面片
    if (nullptr == face)
    {
        for (auto tempFace : qAsConst(_faceList))
        {
            // 找到有交点的面片后退出
            if (tempFace->calcIntersection(clipResult))
            {
                break;
            }
        }
    }
    // 指定面片时直接计算交点
    else face->calcIntersection(clipResult);
}
}
