#include "quadtreedef.h"
#include <QDebug>

/// MPtInfo
MPtInfo::MPtInfo(const int &x, const int &y, const int &index, const int &startType)
{
    _coor_x = x;
    _coor_y = y;
    _x = x * POPO;
    _y = y * POPO;
    _index = index;
    _startType = startType;
}

void MPtInfo::setNode(Node *node) {
    _endNode = node;
}

MPtInfo &MPtInfo::operator=(const MPtInfo &pt) {
    this->_x = pt._x;
    this->_y = pt._y;
    this->_index = pt._index;
    this->_bro = pt._bro;
    this->_endNode = pt._endNode;
    return *this;
}

/// Node
///
/// @brief 四叉树节点构造函数
/// @param parent - 父节点指针
/// @param depth - 节点深度
/// @param nodeType - 节点类型(LEFT/RIGHT/UP/DOWN/UL/UR/DL/DR)
/// @details 初始化节点的基本属性
///
Node::Node(Node *parent, const int &depth, const int &nodeType)
{
    _parent = parent;    // 设置父节点
    _depths = depth;     // 设置深度层级
    _nodeType = nodeType; // 设置节点类型
}

/**
 * @brief 在四叉树中插入节点
 * 
 * @param pt 要插入的点信息指针
 * @param x0 当前节点区域的左上角x坐标
 * @param y0 当前节点区域的左上角y坐标
 * @param width 当前节点区域的宽度
 * @param height 当前节点区域的高度
 * @return bool 插入成功返回true
 */
bool Node::insertNode(const MPtInfoPtr &pt, const int &x0, const int &y0,
                     const int &width, const int &height)
{
    // 增加节点计数
    ++ _nodeCount;

    // 更新当前节点的边界范围
    _minX = x0;
    _minY = y0; 
    _maxX = x0 + width;
    _maxY = y0 + height;

    // 计算区域的一半宽度和高度
    int nHalfWid = width >> 1;
    int nHalfHei = height >> 1;

    // 计算区域中心点坐标
    int centerX = x0 + nHalfWid;
    int centerY = y0 + nHalfHei;

    // 如果当前深度小于最大深度,创建内部节点
    if (_depths < MaxDepths - 1)
    {
        // 根据点的位置判断属于哪个象限
        if (pt->_y < centerY)
        {
            if (pt->_x < centerX) // 左上象限
            {
                // 如果左上子节点不存在则创建
                if (false == _nodeMap.contains(UL))
                {
                    _nodeMap[UL] = QSharedPointer<Node>(new Node(this, _depths + 1, UL));
                }
                // 递归插入左上子树
                _nodeMap[UL]->insertNode(pt, x0, y0, nHalfWid, nHalfHei);
            }
            else // 右上象限
            {
                // 如果右上子节点不存在则创建
                if (false == _nodeMap.contains(UR))
                {
                    _nodeMap[UR] = QSharedPointer<Node>(new Node(this, _depths + 1, UR));
                }
                // 递归插入右上子树
                _nodeMap[UR]->insertNode(pt, x0 + nHalfWid, y0, nHalfWid, nHalfHei);
            }
        }
        else 
        {
            if (pt->_x < centerX) // 左下象限
            {
                // 如果左下子节点不存在则创建
                if (false == _nodeMap.contains(DL))
                {
                    _nodeMap[DL] = QSharedPointer<Node>(new Node(this, _depths + 1, DL));
                }
                // 递归插入左下子树
                _nodeMap[DL]->insertNode(pt, x0, y0 + nHalfHei, nHalfWid, nHalfHei);
            }
            else // 右下象限
            {
                // 如果右下子节点不存在则创建
                if (false == _nodeMap.contains(DR))
                {
                    _nodeMap[DR] = QSharedPointer<Node>(new Node(this, _depths + 1, DR));
                }
                // 递归插入右下子树
                _nodeMap[DR]->insertNode(pt, x0 + nHalfWid, y0 + nHalfHei, nHalfWid, nHalfHei);
            }
        }
    }
    else // 当前深度达到最大深度,创建叶子节点
    {
        // 与上面类似的四象限判断,但创建的是EndNode叶子节点
        if (pt->_y < centerY)
        {
            if (pt->_x < centerX)
            {
                if (false == _nodeMap.contains(UL))
                {
                    _nodeMap[UL] = QSharedPointer<Node>(new EndNode(this, _depths + 1, UL));
                }
                _nodeMap[UL]->insertNode(pt, x0, y0, nHalfWid, nHalfHei);
            }
            else
            {
                if (nullptr == _nodeMap[UR])
                {
                    _nodeMap[UR] = QSharedPointer<Node>(new EndNode(this, _depths + 1, UR));
                }
                _nodeMap[UR]->insertNode(pt, x0 + nHalfWid, y0, nHalfWid, nHalfHei);
            }
        }
        else
        {
            if (pt->_x < centerX)
            {
                if (nullptr == _nodeMap[DL])
                {
                    _nodeMap[DL] = QSharedPointer<Node>(new EndNode(this, _depths + 1, DL));
                }
                _nodeMap[DL]->insertNode(pt, x0, y0 + nHalfHei, nHalfWid, nHalfHei);
            }
            else
            {
                if (nullptr == _nodeMap[DR])
                {
                    _nodeMap[DR] = QSharedPointer<Node>(new EndNode(this, _depths + 1, DR));
                }
                _nodeMap[DR]->insertNode(pt, x0 + nHalfWid, y0 + nHalfHei, nHalfWid, nHalfHei);
            }
        }
    }

    return true;
}


/**
 * @brief 从四叉树中移除指定点
 * 
 * @param pt 要移除的点信息指针
 * @return bool 移除成功返回true
 * 
 * @details 该函数通过递归方式从四叉树中移除点:
 * 1. 如果存在父节点,则递归调用父节点的removePt
 * 2. 减少节点计数
 */
bool Node::removePt(const MPtInfoPtr &pt)
{
    // 如果有父节点,递归调用父节点的移除函数
    if (_parent) _parent->removePt(pt);
    
    // 减少节点计数
    -- _nodeCount;
    
    return true;
}



//////EndNode
/**
 * @brief 在叶子节点中插入点
 * 
 * @param pt 要插入的点信息指针
 * @param x0 叶子节点区域的左上角x坐标
 * @param y0 叶子节点区域的左上角y坐标
 * @param width 叶子节点区域的宽度
 * @param height 叶子节点区域的高度
 * @return bool 插入成功返回true
 * 
 * @details 叶子节点是四叉树的终端节点,直接存储点信息:
 * 1. 更新节点的边界范围
 * 2. 增加节点计数
 * 3. 将点添加到点集合中
 * 4. 设置点所属的节点
 */
bool EndNode::insertNode(const MPtInfoPtr &pt, const int &x0, const int &y0,
    const int &width, const int &height)
{
    // 更新叶子节点的边界范围
    _minX = x0;
    _minY = y0;
    _maxX = x0 + width;
    _maxY = y0 + height;

    // 增加节点计数
    ++ _nodeCount;

    // 将点添加到点集合中
    _ptInfoSets.insert(pt);

    // 设置点所属的节点为当前节点
    pt->setNode(this);

    return true;
}

/**
 * @brief 从叶子节点中移除指定点
 * 
 * @param pt 要移除的点信息指针
 * @return bool 移除成功返回true
 * 
 * @details 叶子节点的点移除流程:
 * 1. 检查点是否在当前节点的点集合中
 * 2. 如果存在父节点,递归调用父节点的removePt
 * 3. 减少节点计数
 * 4. 从点集合中移除该点
 */
bool EndNode::removePt(const MPtInfoPtr &pt)
{
    // 检查点是否在当前节点的点集合中
    if (_ptInfoSets.contains(pt))
    {
        // 如果有父节点,递归调用父节点的移除函数
        if (_parent) _parent->removePt(pt);

        // 减少节点计数
        -- _nodeCount;
        
        // 从点集合中移除该点
        _ptInfoSets.remove(pt);
    }
    
    return true;
}



/// SearchResult
///
SearchResult::SearchResult(const MPtInfoPtr &ptInfoPtr) :
    _referPt(ptInfoPtr)
{ }

/**
 * @brief 计算最近点
 * 
 * @param node 当前搜索的节点指针
 * 
 * @details 递归搜索四叉树找到距离参考点最近的点:
 * 1. 参数有效性检查
 * 2. 叶子节点处理:计算点集中各点到参考点的距离
 * 3. 内部节点处理:递归搜索子节点
 */
void SearchResult::calcNearestPt(Node *node)
{
    // 参数有效性检查
    if (nullptr == _referPt) return;
    if (nullptr == node) return;
    if (node->_nodeCount < 1) return;

    // 到达叶子节点层
    if (node->_depths == MaxDepths)
    {
        // 转换为叶子节点类型
        if(auto endNode = dynamic_cast<EndNode *>(node))
        {
            // 遍历叶子节点中的所有点
            for (const auto &pt : qAsConst(endNode->_ptInfoSets))
            {
                // 计算当前点到参考点的欧氏距离平方
                auto tempDis = pow(pt->_coor_x - _referPt->_coor_x, 2) +
                              pow(pt->_coor_y - _referPt->_coor_y, 2);
                
                // 更新最小距离和最近点
                if (tempDis < _minDis)
                {
                    _minDis = tempDis;
                    _startPt = pt;
                }
            }
        }
    }
    else // 内部节点
    {
        // 递归搜索所有子节点
        for (const auto &item : qAsConst(node->_nodeMap))
        {
            calcNearestPt(item.data());
        }
    }
}

/// NodeSearcher
/**
 * @brief NodeSearcher类构造函数
 * 
 * @param node 要搜索的节点指针
 * @param searchType 搜索类型
 * @details 初始化节点搜索器,设置起始节点和搜索类型
 */
NodeSearcher::NodeSearcher(Node *node, const int &searchType) :
    _node(node),
    _curSearchType(searchType)
{
}

/**
 * @brief 获取下一个搜索节点类型
 * 
 * @param nodeType 当前节点类型
 * @param searchType 搜索类型(入参和出参)
 * @return int 返回下一个节点类型
 * 
 * @details 通过位运算确定搜索方向:
 * 1. 0x11处理左右方向
 * 2. 0x1100处理上下方向
 * 3. 更新搜索类型
 */
int NodeSearcher::getNodeType(const int &nodeType, int &searchType)
{
    // 计算下一个节点类型
    int nextNodeType = nodeType;
    
    // 处理左右方向的搜索(0x11)
    if (searchType & 0x11) 
        nextNodeType = (nextNodeType & 0x1100) | (~nextNodeType & 0x11);
    
    // 处理上下方向的搜索(0x1100)
    if (searchType & 0x1100) 
        nextNodeType = (~nextNodeType & 0x1100) | (nextNodeType & 0x11);
    
    // 更新搜索类型
    searchType = searchType & nodeType;
    
    return nextNodeType;
}

/**
 * @brief 查找临近节点
 * 
 * @param searchedInfo 搜索结果信息
 * @return bool 查找成功返回true,失败返回false
 * 
 * @details 节点查找过程:
 * 1. 向上搜索阶段(_curSearchType不为0):
 *    - 计算并记录下一个搜索节点类型
 *    - 移动到父节点继续搜索
 * 
 * 2. 向下搜索阶段(_curSearchType为0):
 *    - 检查节点有效性
 *    - 获取下一个要搜索的节点类型
 *    - 移动到对应的子节点
 *    - 到达目标节点时计算最近点
 */
bool NodeSearcher::findNearNode(SearchResult &searchedInfo)
{
    // 检查节点有效性
    if (nullptr == _node) return false;

    // 向上搜索阶段
    if (_curSearchType)
    {
        // 记录下一个搜索节点类型
        _nodeTypeList << getNodeType(_node->_nodeType, _curSearchType);
        // 移动到父节点
        _node = _node->_parent;
    }
    // 向下搜索阶段
    else
    {
        // 检查节点计数
        if (_node->_nodeCount < 1) return false;

        // 获取下一个要搜索的节点类型
        auto useNodeType = _nodeTypeList.takeLast();
        // 移动到对应的子节点
        _node = _node->_nodeMap.value(useNodeType, QSharedPointer<Node>()).data();

        // 搜索列表为空且当前节点有效时,计算最近点
        if ((_nodeTypeList.size() < 1) && _node && _node->_nodeCount)
        {
            searchedInfo.calcNearestPt(_node);
        }
        
        return _nodeTypeList.size();
    }

    return true;
}


/**
 * @brief 从根节点开始查找最近点
 * 
 * @param node 当前节点指针
 * @param searchedInfo 搜索结果信息
 * @return bool 查找成功返回true,失败返回false
 * 
 * @details 从根节点递归查找的流程:
 * 1. 基本检查:
 *    - 节点有效性检查
 *    - 节点计数检查
 *    - 到达最大深度时计算最近点
 * 
 * 2. 区域判断:
 *    - 判断参考点是否在当前节点区域内
 *    - 根据参考点位置决定搜索顺序
 * 
 * 3. 递归搜索:
 *    - 按照优先级顺序搜索四个象限
 *    - 根据搜索结果决定是否继续其他象限
 */
bool NodeSearcher::findNearNode_root(Node *node, SearchResult &searchedInfo)
{
    // 基本检查
    if (nullptr == node) return false;
    if (node->_nodeCount < 1) return false;
    
    // 到达最大深度,计算最近点
    if (MaxDepths == node->_depths)
    {
        searchedInfo.calcNearestPt(node);
        return true;
    }

    // 判断参考点是否在节点区域内
    if (searchedInfo._referPt->_x >= node->_minX && searchedInfo._referPt->_x < node->_maxX
        && searchedInfo._referPt->_y >= node->_minY && searchedInfo._referPt->_x < node->_maxY)
    {
        // 按顺序搜索四个象限
        findNearNode_root(node, UL, searchedInfo);
        findNearNode_root(node, DL, searchedInfo);
        findNearNode_root(node, UR, searchedInfo);
        findNearNode_root(node, DR, searchedInfo);
    }

    // 搜索所有象限
    findNearNode_root(node, UL, searchedInfo);
    findNearNode_root(node, DL, searchedInfo);
    findNearNode_root(node, UR, searchedInfo);
    findNearNode_root(node, DR, searchedInfo);
    return true;

    // 根据参考点位置选择搜索顺序
    if (searchedInfo._referPt->_x < node->_minX)
    {
        if (searchedInfo._referPt->_y < node->_minY)
        {
            if(false == findNearNode_root(node, UL, searchedInfo))
            {
                findNearNode_root(node, UR, searchedInfo);
                findNearNode_root(node, DL, searchedInfo);
                findNearNode_root(node, DR, searchedInfo);
            }
        }
        else if (searchedInfo._referPt->_y >= node->_maxY)
        {
            if(false == findNearNode_root(node, DL, searchedInfo))
            {
                findNearNode_root(node, UL, searchedInfo);
                findNearNode_root(node, UR, searchedInfo);
                findNearNode_root(node, DR, searchedInfo);
            }
        }
        else
        {
            bool result_1 = findNearNode_root(node, UL, searchedInfo);
            bool result_2 = findNearNode_root(node, DL, searchedInfo);
            if (false == result_1 && false == result_2)
            {
                findNearNode_root(node, UR, searchedInfo);
                findNearNode_root(node, DR, searchedInfo);
            }
        }
    }
    else if (searchedInfo._referPt->_x >= node->_maxX)
    {
        if (searchedInfo._referPt->_y < node->_minY)
        {
            if (false == findNearNode_root(node, UR, searchedInfo))
            {
                findNearNode_root(node, UL, searchedInfo);
                findNearNode_root(node, DL, searchedInfo);
                findNearNode_root(node, DR, searchedInfo);
            }
        }
        else if (searchedInfo._referPt->_y >= node->_maxY)
        {
            if (findNearNode_root(node, DR, searchedInfo))
            {
                findNearNode_root(node, UL, searchedInfo);
                findNearNode_root(node, DL, searchedInfo);
                findNearNode_root(node, UR, searchedInfo);
            }
        }
        else
        {
            bool result_1 = findNearNode_root(node, UR, searchedInfo);
            bool result_2 = findNearNode_root(node, DR, searchedInfo);
            if (false == result_1 && false == result_2)
            {
                findNearNode_root(node, UL, searchedInfo);
                findNearNode_root(node, DL, searchedInfo);
            }
        }
    }
    else
    {
        if (searchedInfo._referPt->_y < node->_minY)
        {
            bool result_1 = findNearNode_root(node, UL, searchedInfo);
            bool result_2 = findNearNode_root(node, UR, searchedInfo);
            if (false == result_1 && false == result_2)
            {
                findNearNode_root(node, DL, searchedInfo);
                findNearNode_root(node, DR, searchedInfo);
            }
        }
        else if (searchedInfo._referPt->_y >= node->_maxY)
        {
            bool result_1 = findNearNode_root(node, DL, searchedInfo);
            bool result_2 = findNearNode_root(node, DR, searchedInfo);
            if (false == result_1 && false == result_2)
            {
                findNearNode_root(node, UL, searchedInfo);
                findNearNode_root(node, UR, searchedInfo);
            }
        }
        else
        {
            findNearNode_root(node, UL, searchedInfo);
            findNearNode_root(node, DL, searchedInfo);
            findNearNode_root(node, UR, searchedInfo);
            findNearNode_root(node, DR, searchedInfo);
        }
    }
    return true;
}

/**
 * @brief 在指定方向搜索最近点
 * 
 * @param node 当前节点指针
 * @param direct 搜索方向(UL/UR/DL/DR)
 * @param searchedInfo 搜索结果信息
 * @return bool 搜索成功返回true,失败返回false
 * 
 * @details 定向搜索流程:
 * 1. 检查指定方向是否存在子节点
 * 2. 检查子节点的节点计数
 * 3. 递归搜索子节点
 */
bool NodeSearcher::findNearNode_root(Node *node, const int &direct, SearchResult &searchedInfo)
{
    // 检查指定方向是否存在子节点
    if (false == node->_nodeMap.contains(direct)) return false;
    
    // 检查子节点的节点计数
    if (node->_nodeMap[direct]->_nodeCount < 1) return false;

    // 递归搜索子节点
    findNearNode_root(node->_nodeMap[direct].data(), searchedInfo);
    
    return true;
}



///////////
///MQuadTree
/**
 * @brief 向四叉树中插入节点
 * 
 * @param pt 要插入的点信息指针
 * @return bool 插入成功返回true
 * 
 * @details 从根节点开始插入:
 * 1. 起始坐标为(0,0)
 * 2. 区域大小为2^MaxDepths
 * 3. 通过根节点递归插入
 */
bool MQuadTree::insertNode(const MPtInfoPtr &pt)
{
    // 从根节点开始插入,初始区域为2^MaxDepths大小的正方形
    return _root->insertNode(pt, 0, 0, (1 << MaxDepths), (1 << MaxDepths));
}


/**
 * @brief 使用四叉树对路径进行排序
 * 
 * @param curPt 当前点信息指针
 * @param srcPath 源路径集合
 * @param desPath 目标路径集合
 * @return bool 排序成功返回true
 * 
 * @details 排序流程:
 * 1. 更新初始路径信息
 * 2. 检查树节点数量
 * 3. 循环处理:
 *    - 检查剩余节点
 *    - 搜索最近邻点
 *    - 更新路径信息
 */
bool MQuadTree::sortPaths(const MPtInfoPtr &curPt, const Paths &srcPath, Paths &desPath)
{
    // 更新初始路径信息
    updatePathInfo(curPt, srcPath, desPath);

    // 检查树是否为空
    if (_root->_nodeCount < 1) return false;

    // 循环处理所有兄弟节点
    while (_curBroPtr)
    {
        // 检查剩余节点数
        if (getNodeCount() < 1) break;

        // 创建搜索结果对象
        SearchResult searchedInfo(_curBroPtr);
        // 搜索最近邻点
        seekingNeighhorhood(_curBroPtr->_endNode, searchedInfo);

        // NodeSearcher searcher(_root.data(), UL);
        // searcher.findNearNode_root(_root.data(), searchedInfo);

        // if (searchedInfo._startPt)
        //     qDebug() << searchedInfo._startPt->_index
        //              << "\t" << searchedInfo._minDis
        //              << "\t" << searchedInfo._startPt->_coor_x
        //              << "\t" << searchedInfo._startPt->_coor_y
        //              << "\t" << searchedInfo._startPt->_bro.toStrongRef()->_coor_x
        //              << "\t" << searchedInfo._startPt->_bro.toStrongRef()->_coor_y;

        if (false == updatePathInfo(searchedInfo._startPt, srcPath, desPath))
        {
            qDebug() << "!!!--- Error Detected ---!!!" << getNodeCount() << searchedInfo._minDis;
            break;
        }
    }
    // qDebug() << "getNodeCount" << getNodeCount();
    return false;
}

/**
 * @brief 搜索邻近节点
 * 
 * @param node 当前节点指针
 * @param searchedInfo 搜索结果信息
 * 
 * @details 邻近节点搜索流程:
 * 1. 计算当前节点的最近点
 * 2. 向八个方向递归搜索:
 *    - 四个对角方向(UL/UR/DL/DR)
 *    - 四个正交方向(LEFT/RIGHT/UP/DOWN)
 * 3. 找到最近点后终止搜索
 * 4. 未找到时向父节点回溯
 */
void MQuadTree::seekingNeighhorhood(Node *node, SearchResult &searchedInfo)
{
    // 计算当前节点的最近点
    if (node) searchedInfo.calcNearestPt(node);

    // 循环搜索直到找到最近点或到达根节点
    while(node)
    {
        // 搜索八个方向
        seekingNeighhorhood(node, UL, searchedInfo);    // 左上
        seekingNeighhorhood(node, UR, searchedInfo);    // 右上
        seekingNeighhorhood(node, DL, searchedInfo);    // 左下
        seekingNeighhorhood(node, DR, searchedInfo);    // 右下
        seekingNeighhorhood(node, LEFT, searchedInfo);  // 左
        seekingNeighhorhood(node, RIGHT, searchedInfo); // 右
        seekingNeighhorhood(node, UP, searchedInfo);    // 上
        seekingNeighhorhood(node, DOWN, searchedInfo);  // 下

        // 找到最近点后终止搜索
        if (searchedInfo._startPt) break;

        // 向父节点回溯
        node = node->_parent;
    }
}

/**
 * @brief 在指定方向搜索邻近节点
 * 
 * @param node 当前节点指针
 * @param searchType 搜索方向类型
 * @param searchedInfo 搜索结果信息
 */
void MQuadTree::seekingNeighhorhood(Node *node, const int &searchType, SearchResult &searchedInfo)
{
    // 创建节点搜索器并执行搜索
    NodeSearcher searcher(node, searchType);
    while(searcher.findNearNode(searchedInfo));
}

/**
 * @brief 从四叉树中移除点
 * 
 * @param pt 要移除的点信息指针
 * @return bool 移除成功返回true
 */
bool MQuadTree::removePt(const MPtInfoPtr &pt)
{
    // 移除兄弟节点
    if (auto pt2 = pt->_bro.toStrongRef())
    {
        if (pt2->_endNode) pt2->_endNode->removePt(pt2);
    }
    // 移除当前节点
    if (pt->_endNode) pt->_endNode->removePt(pt);
    return true;
}

/**
 * @brief 更新路径信息
 * 
 * @param ptPtr 点信息指针
 * @param srcPaths 源路径集合
 * @param desPaths 目标路径集合
 * @return bool 更新成功返回true
 * 
 * @details 更新流程:
 * 1. 验证点信息有效性
 * 2. 移除当前点
 * 3. 根据起点类型决定路径方向
 * 4. 添加路径到目标集合
 */
bool MQuadTree::updatePathInfo(const MPtInfoPtr &ptPtr,
                              const Paths &srcPaths, Paths &desPaths)
{
    // 验证当前点和兄弟点
    _curPtPtr = ptPtr;
    if (nullptr == _curPtPtr) return false;
    _curBroPtr = _curPtPtr->_bro.toStrongRef();
    if (nullptr == _curBroPtr) return false;

    // 从树中移除当前点
    removePt(_curPtPtr);

    // 检查索引有效性
    if (ptPtr->_index >= srcPaths.size()) return false;

    // 根据起点类型处理路径
    if (ptPtr->_startType)
    {
        // 反向添加路径
        for (int index = _curBroPtr->_index; index <= _curPtPtr->_index; ++ index)
        {
            auto path = srcPaths.at(index);
            ReversePath(path);
            desPaths.push_back(path);
        }
    }
    else
    {
        // 正向添加路径
        for (int index = _curPtPtr->_index; index <= _curBroPtr->_index; ++ index)
        {
            desPaths.push_back(srcPaths.at(index));
        }
    }

    return true;
}
