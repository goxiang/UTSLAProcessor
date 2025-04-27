#include "algorithmhatchingring.h"
#include "slaprocessorextend.h"
#include "UTSLAProcessor_global.h"
#include "uspfiledef.h"
#include <QDebug>
#include <QThread>

/**
 * @brief 窗口位置结构体
 * @param beginPos_1 第一段起始位置
 * @param endPos_1 第一段结束位置  
 * @param beginPos_2 第二段起始位置
 * @param endPos_2 第二段结束位置
 */
struct WndPos {
    uint beginPos_1  = 0;
    uint endPos_1    = 0;
    uint beginPos_2  = 0;
    uint endPos_2   = 0;

    void initValue() {
        beginPos_1  = 0;
        endPos_1    = 0;
        beginPos_2  = 0;
        endPos_2   = 0;
    }
};

inline QDebug operator<<(QDebug debug, const WndPos &wndPos)
{
    debug << "WndPos" << "{ ";
    debug << wndPos.beginPos_1 << wndPos.endPos_1 << wndPos.beginPos_2 << wndPos.endPos_2;
    debug << " }";
    return debug;
}

/**
 * @brief 路径范围结构体
 * @param nInvert 是否反向
 * @param beginPos 起始位置
 * @param endPos 结束位置
 * @param maxSz 最大尺寸
 */
struct PathRang {
    int nInvert = 0;
    uint beginPos = 0;
    uint endPos = 0;
    uint maxSz = 0;
    PathRang(const int &invert, const uint &begin, const uint &end, const uint &sz) {
        nInvert = invert;
        beginPos = begin;
        endPos = end;
        maxSz = sz;
    }
};

/**
 * @brief 大窗口结构体
 * @param nIndex 索引
 * @param lpRange 路径范围指针
 * @param biggerWndPos 大窗口位置
 * @param smallWndPos 小窗口位置
 */
struct BiggerWnd {
    int nIndex = 0;
    PathRang *lpRange = nullptr;
    WndPos biggerWndPos;
    WndPos smallWndPos;
    BiggerWnd(const int &index, PathRang *range) {
        nIndex = index;
        lpRange = range;
    }
};

inline QDebug operator<<(QDebug debug, const BiggerWnd &wnd)
{
    debug << "{ " << wnd.biggerWndPos.beginPos_1 << ", " << wnd.biggerWndPos.endPos_1 << " }";
    return debug;
}

/**
 * @brief 大窗口比较函数
 * 
 * @param wnd1 输入参数,第一个大窗口
 * @param wnd2 输入参数,第二个大窗口
 * @return true wnd1小于wnd2
 * @return false wnd1大于等于wnd2
 * 
 * @details 比较流程:
 * 1. 获取路径范围和窗口位置信息
 * 2. 根据反向标志选择比较方式
 * 3. 正向时比较起始位置
 * 4. 反向时比较结束位置
 */
bool lessThanBiggerWnd(const BiggerWnd &wnd1, const BiggerWnd &wnd2) {
    const auto lpRange = wnd1.lpRange;
    const auto &biggerWndPos1 = wnd1.biggerWndPos;
    const auto &biggerWndPos2 = wnd2.biggerWndPos;

    // 根据方向进行比较
    if(0 == lpRange->nInvert) {
        // 正向比较起始位置
        if(lpRange->beginPos < lpRange->endPos) {
            return biggerWndPos1.beginPos_1 < biggerWndPos2.beginPos_1;
        }
        else {
            if(biggerWndPos1.beginPos_1 <= lpRange->endPos) {
                if(biggerWndPos2.beginPos_1 <= lpRange->endPos) {
                    return biggerWndPos1.beginPos_1 < biggerWndPos2.beginPos_1;
                }
                else return false;
            }
            else {
                if(biggerWndPos2.beginPos_1 <= lpRange->endPos) {
                    return true;
                }
                else return biggerWndPos1.beginPos_1 > biggerWndPos2.beginPos_1;
            }
        }
    }
    else {
        // 反向比较结束位置
        if(lpRange->beginPos < lpRange->endPos) {
            return biggerWndPos1.beginPos_1 > biggerWndPos2.beginPos_1;
        }
        else {
            if(biggerWndPos1.beginPos_1 <= lpRange->endPos) {
                if(biggerWndPos2.beginPos_1 <= lpRange->endPos) {
                    return biggerWndPos1.beginPos_1 > biggerWndPos2.beginPos_1;
                }
                else return true;
            }
            else {
                if(biggerWndPos2.beginPos_1 <= lpRange->endPos) {
                    return false;
                }
                else return biggerWndPos1.beginPos_1 < biggerWndPos2.beginPos_1;
            }
        }
    }
}

void drawPathOffset(const Path &path, const IntPoint &offsetCoor, const ulong &ms);

/**
 * @brief 路径分支面积升序比较
 * 
 * @param b1 输入参数,第一个分支
 * @param b2 输入参数,第二个分支
 * @return true b1面积小于b2
 * @return false b1面积大于等于b2
 */
bool compareLessThan(const PocketBranch &b1, const PocketBranch &b2)
{
    return fabs(b1.maxArea()) < fabs(b2.maxArea());
}

/**
 * @brief 路径分支面积降序比较
 * 
 * @param b1 输入参数,第一个分支
 * @param b2 输入参数,第二个分支
 * @return true b1面积大于b2
 * @return false b1面积小于等于b2
 */
bool compareBigger(const PocketBranch &b1, const PocketBranch &b2)
{
    return fabs(b1.maxArea()) > fabs(b2.maxArea());
}

/**
 * @brief 路径分支综合比较函数
 * 
 * @param b1 输入参数,第一个分支
 * @param b2 输入参数,第二个分支
 * @return true 当b1优先级高于b2时返回true
 * @return false 当b1优先级低于或等于b2时返回false
 * 
 * @details 比较流程:
 * 1. 首先比较轮廓类型(mCircleType)
 *    - 不同类型按类型值升序排序（先内轮廓后外轮廓）
 * 2. 类型相同时比较面积
 *    - 按面积绝对值降序排序
 */
bool compareTotalBranch(const PocketBranch &b1, const PocketBranch &b2)
{
    // 1. 比较轮廓类型
    if(b1.mCircleType != b2.mCircleType)
    {
        return b1.mCircleType < b2.mCircleType;
    }
    // 2. 比较面积大小
    else
    {
        return fabs(b1.maxArea()) > fabs(b2.maxArea());
    }
}

/**
 * @brief 路径分支窗口位置比较函数
 * 
 * @param b1 输入参数,第一个分支
 * @param b2 输入参数,第二个分支
 * @return true 当b1的窗口起始位置小于b2时返回true
 * @return false 当b1的窗口起始位置大于等于b2时返回false
 * 
 * @details 比较流程:
 * 1. 获取两个分支的第一个节点
 * 2. 比较节点中大窗口(bigWnd)的起始位置(nWndBegin)
 * 3. 用于按窗口位置对分支进行排序
 */
bool compareTotalBranchWnd(const PocketBranch &b1, const PocketBranch &b2)
{
    return b1.mNodeList.first().ringWndInfo.bigWnd.nWndBegin < b2.mNodeList.first().ringWndInfo.bigWnd.nWndBegin;
}

/**
 * @brief PocketNode构造函数
 * 
 * @param level 输入参数,节点层级
 * @param area 输入参数,区域面积
 * @param box 输入参数,边界框
 * @param path 输入参数,轮廓路径
 * 
 * @details 初始化流程:
 * 1. 设置节点层级
 * 2. 设置区域面积
 * 3. 设置边界框
 * 4. 设置轮廓路径
 */
PocketNode::PocketNode(const int &level, const double &area,
           const BoundingBox &box, const Path &path)
{
    nLevel = level;
    fArea = area;
    bBox = box;
    circle = path;
}

/**
 * @brief 获取轮廓第一个坐标点
 * @return IntPoint 返回轮廓起点坐标
 */
IntPoint PocketNode::firstCoor() const
{
    return circle.front();
}

/**
 * @brief 检查点是否在轮廓内
 * 
 * @param coor 输入参数,待检查的坐标点
 * @return true 点在轮廓内
 * @return false 点不在轮廓内
 * 
 * @details 检查流程:
 * 1. 先用边界框快速判断
 * 2. 通过多边形内点算法精确判断
 */
bool PocketNode::containsCoor(const IntPoint &coor) const
{
    if(bBox.coorInBox(coor)) return (1 == PointInPolygon(coor, circle));
    return false;
}

/**
 * @brief PocketBranch构造函数
 * @param circleType 输入参数,轮廓类型(Inner/Outer)
 * @param node 输入参数,初始节点
 * @details 初始化流程:
 * 1. 设置轮廓类型(内轮廓/外轮廓)
 * 2. 将初始节点添加到节点列表
 */
PocketBranch::PocketBranch(const qint8 &circleType, const PocketNode &node)
{
    mCircleType = circleType;    // 设置轮廓类型
    mNodeList << node;           // 添加初始节点
}


/**
 * @brief 检查节点是否包含指定坐标点
 * @param coor 输入参数,待检查的坐标点
 * @param index 输入参数,节点索引
 * @return true 坐标点在节点内
 * @return false 坐标点不在节点内
 * @details 检查流程:
 * 1. 有子分支时:
 *    - 递归检查所有子分支
 *    - 任一子分支包含该点则返回true
 * 2. 无子分支时:
 *    - 检查最后一个节点是否包含该点
 *    - 包含则记录索引并返回true
 * 3. 都不包含则返回false
 */
bool PocketBranch::containsNode(const IntPoint &coor, const int &index)
{
    // 1. 检查子分支
    if(mSubBranchList.size())
    {
        for(auto &subBranch : mSubBranchList)
        {
            if(subBranch.containsNode(coor, index)) 
                return true;
        }
    }
    // 2. 检查当前节点
    else
    {
        if(mNodeList.last().containsCoor(coor))
        {
            mTempIndexList << index;
            return true;
        }
    }
    // 3. 不包含返回false
    return false;
}


/**
 * @brief 检查节点是否在指定节点内部
 * 
 * @param node 输入输出参数,待检查的节点
 * @param index 输入参数,分支索引
 * @return true 当前分支的起点在node内
 * @return false 当前分支的起点不在node内
 * 
 * @details 检查流程:
 * 1. 获取当前分支第一个节点的起点坐标
 * 2. 检查该坐标是否在输入节点内
 * 3. 如果在内部:
 *    - 记录分支索引到节点的子分支列表
 *    - 返回true
 * 4. 不在内部返回false
 */
bool PocketBranch::containsInNode(PocketNode &node, const int &index) const
{
    // 检查当前分支起点是否在node内
    if(node.containsCoor(this->mNodeList.first().firstCoor()))
    {
        node.subBranchList << index;  // 记录分支索引
        return true;
    }
    return false;
}

/**
 * @brief 更新嵌套关系列表
 * 
 * @param thisIndex 输入参数,当前分支索引
 * @param branch 输入输出参数,待比较分支
 * @param subIndex 输入参数,待比较分支索引
 * 
 * @details 
    外轮廓
    ├── 内孔1
    │     └── 岛1
    └── 内孔2
            ├── 岛2
            └── 岛3
 */
void PocketBranch::updateNestingList(const int &thisIndex, PocketBranch &branch, const int &subIndex)
{
    if(fabs(branch.mNodeList.last().fArea) > fabs(this->mNodeList.last().fArea))
    {
        // 大轮廓包含小轮廓的起点
        if(branch.mNodeList.last().containsCoor(this->mNodeList.last().firstCoor())) branch.mNestingList << thisIndex;
    }
    else
    {
        if(this->mNodeList.last().containsCoor(branch.mNodeList.last().firstCoor())) this->mNestingList << subIndex;
    }
}

/**
 * @brief 添加节点到分支结构
 * @param tempNodeList 输入参数,临时节点列表
 * @details 添加流程:
 * 1. 存在子分支时:
 *    - 递归对每个子分支调用addNode
 * 2. 无子分支但有临时索引时:
 *    - 单个索引:直接添加节点到当前节点列表
 *    - 多个索引:为每个索引创建新的子分支
 * 3. 清理临时索引列表
 */
void PocketBranch::addNode(QList<PocketNode> &tempNodeList)
{
    // 1. 递归处理子分支
    if(mSubBranchList.size())
    {
        for(auto &branch : mSubBranchList) 
            branch.addNode(tempNodeList);
    }
    // 2. 处理临时索引
    else if(mTempIndexList.size())
    {
        if(1 == mTempIndexList.size()) 
            // 单个索引直接添加节点
            mNodeList << tempNodeList.at(int(mTempIndexList.at(0)));
        else
        {
            // 多个索引创建子分支
            for(const auto &index : mTempIndexList)
            {
                mSubBranchList << PocketBranch(mCircleType, tempNodeList.at(int(index)));
            }
        }
        // 3. 清理临时索引
        mTempIndexList.clear();
    }
}


/**
 * @brief 添加内部分支到当前分支结构
 * @param innerBranch 输入参数,待添加的内部分支
 * @return true 成功添加内部分支
 * @return false 未找到合适位置添加
 * @details 添加流程:
 * 1. 遍历当前分支的子分支:
 *    - 跳过内轮廓类型的子分支
 *    - 递归尝试添加到外轮廓子分支
 *    - 任一子分支添加成功则返回true
 * 2. 检查当前节点是否可以添加:
 *    - 层级相同
 *    - 当前节点包含内部分支的起点
 *    - 满足条件则在子分支列表开头插入
 * 3. 都不满足则返回false
 */
bool PocketBranch::addInnerBranch(PocketBranch &innerBranch)
{
    // 1. 递归检查子分支
    for(auto &branch : mSubBranchList)
    {
        if(Inner == branch.mCircleType) continue;
        if(branch.addInnerBranch(innerBranch)) return true;
    }

    // 2. 检查当前节点
    const auto &curNode = this->mNodeList.last();
    const auto &innerNode = innerBranch.mNodeList.first();
    if(curNode.nLevel == innerNode.nLevel && 
       curNode.containsCoor(innerNode.firstCoor()))
    {
        this->mSubBranchList.insert(0, innerBranch);
        return true;
    }

    // 3. 未找到合适位置
    return false;
}


/**
 * @brief 升级版添加内部分支函数
 * 
 * @param innerBranch 输入参数,待添加的内部分支
 * @return true 成功添加内部分支
 * @return false 未找到合适位置添加
 * 
 * @details 处理流程:
 * 1. 层级检查:
 *    - 获取内部分支的层级
 *    - 获取当前分支的起始和结束层级
 *    - 检查内部分支层级是否在范围内
 * 
 * 2. 位置检查:
 *    - 获取对应层级的当前节点
 *    - 检查是否包含内部分支起点
 * 
 * 3. 分支分割:
 *    - 如果不是最后层级,需要分割当前分支
 *    - 创建新的分割分支
 *    - 移动高于插入层级的节点到分割分支
 *    - 移动相关子分支到分割分支
 * 
 * 4. 分支插入:
 *    - 插入分割分支(如果有)
 *    - 插入内部分支
 * 
 * 5. 递归处理:
 *    - 如果当前位置不合适,递归检查子分支
 */
bool PocketBranch::addInnerBranch_upgrade(PocketBranch &innerBranch)
{
    // 1. 获取层级信息
    const auto innerNode = innerBranch.mNodeList.first();
    const int nInnerLevel = innerNode.nLevel;
    const int nStartLevel = this->mNodeList.first().nLevel;
    const int nEndLevel = this->mNodeList.last().nLevel;

    // 2. 检查层级范围和位置
    if(nInnerLevel >= nStartLevel && nInnerLevel <= nEndLevel)
    {
        const auto &curNode = this->mNodeList.at(nInnerLevel - nStartLevel);
        if(curNode.containsCoor(innerNode.firstCoor())) {
            // 3. 需要分割处理
            if(nInnerLevel != nEndLevel) {
                // 创建分割分支
                PocketBranch splitBranch;
                splitBranch.mCircleType = Outer;
                
                // 移动节点
                int nSplitPos = nInnerLevel - nStartLevel + 1;
                for(int iLevel = nSplitPos; iLevel < nEndLevel - nStartLevel + 1; ++ iLevel)
                {
                    splitBranch.mNodeList << mNodeList.takeAt(nSplitPos);
                }

                // 移动子分支
                int iSub = 0;
                while(iSub < mSubBranchList.size())
                {
                    if(mSubBranchList[iSub].mCircleType || 
                       (mSubBranchList[iSub].mNodeList.first().nLevel > nInnerLevel))
                    {
                        splitBranch.mSubBranchList << mSubBranchList[iSub];
                        mSubBranchList.takeAt(iSub);
                        continue;
                    }
                    ++ iSub;
                }
                
                // 插入分割分支
                this->mSubBranchList.insert(0, splitBranch);
            }
            
            // 4. 插入内部分支
            this->mSubBranchList.insert(0, innerBranch);
            return true;
        }
    }

    // 5. 递归处理子分支
    for(auto &branch : mSubBranchList)
    {
        if(Inner == branch.mCircleType) continue;
        if(branch.addInnerBranch_upgrade(innerBranch)) return true;
    }

    return false;
}


/**
 * @brief 更新分支顺序
 * 
 * @details 处理流程:
 * 1. 对子分支列表排序:
 *    - 使用compareTotalBranch比较函数
 *    - 先按轮廓类型排序
 *    - 同类型按面积降序排序
 * 2. 递归更新所有子分支的顺序
 */
void PocketBranch::updateBranchOrder()
{
    std::sort(mSubBranchList.begin(), mSubBranchList.end(), compareTotalBranch);
    for(auto &branch : mSubBranchList)
    {
        branch.updateBranchOrder();
    }
}

/**
 * @brief 获取分支最大面积
 * @return double 返回第一个节点的面积
 */
double PocketBranch::maxArea() const
{
    return mNodeList.first().fArea;
}

/**
 * @brief 获取节点总数
 * @param nodeCnt 输出参数,累计节点数量
 * @details 统计流程:
 * 1. 累加当前分支的节点数
 * 2. 递归累加所有子分支的节点数
 */
void PocketBranch::getNodeCnt(int &nodeCnt) const
{
    nodeCnt += mNodeList.size();
    for(const auto &branch : mSubBranchList)
    {
        branch.getNodeCnt(nodeCnt);
    }
}

/**
 * @brief 逆时针计算路径终点位置并插入过渡点
 * 
 * @param path 输入输出参数,路径点集
 * @param beginPos 输入输出参数,起始位置
 * @param endPos 输入输出参数,结束位置
 * @param powDis 输入参数,距离阈值的平方
 * @return uint 返回插入点的位置,无插入则返回0xFFFFFFFF
 */
uint calcEndingPos(Path &path, uint &beginPos, uint &endPos, const double &powDis)
{
    uint insertedPos = 0xFFFFFFFF;
    const auto beginCoor = path[beginPos];
    uint coorCnt = path.size();
    double fTempDis = 0.0;
    // double fTempDis1 = 0.0;

    // 向前搜索终点
    endPos = beginPos;
    do
    {
        -- endPos;
        if(endPos > coorCnt - 1) endPos = coorCnt - 1;
        // 处理起点终点重合
        if(endPos == beginPos) {
            endPos = beginPos - 1;
            if(endPos > coorCnt - 1) endPos = coorCnt - 1;
            break;
        }
        // 计算距离
        fTempDis = (path[endPos].X - beginCoor.X) * (path[endPos].X - beginCoor.X) +
                (path[endPos].Y - beginCoor.Y) * (path[endPos].Y - beginCoor.Y);
        // 距离超过阈值
        if(fTempDis > powDis) {
            double fMaxDis = 1.5 * powDis;
            // 需要插入过渡点
            if(powDis > 100 && (fTempDis > fMaxDis)) {
                // 计算前一点位置和距离
                uint prePos = endPos + 1;
                if(prePos > coorCnt - 1) prePos = 0;
                double fTempDisPre = (path[prePos].X - beginCoor.X) * (path[prePos].X - beginCoor.X) +
                        (path[prePos].Y - beginCoor.Y) * (path[prePos].Y - beginCoor.Y);
                // 计算插值比例
                double fRatio = sqrt((powDis - fTempDisPre) / (fTempDis - fTempDisPre));
                // 计算过渡点坐标
                const auto tempCoor = IntPoint(qRound64(path[prePos].X + (path[endPos].X - path[prePos].X) * fRatio),
                                               qRound64(path[prePos].Y + (path[endPos].Y - path[prePos].Y) * fRatio));
                // 插入过渡点
                path.insert(path.begin() + int(endPos + 1), tempCoor);
                insertedPos = endPos;
                // 更新位置索引
                ++ coorCnt;
                if(beginPos >= insertedPos) {
                    ++ beginPos;
                    if(beginPos > coorCnt - 1) beginPos = 0;
                }
                ++ endPos;
                if(endPos > coorCnt - 1) endPos = 0;

//                fTempDis1 = (tempCoor.X - beginCoor.X) * (tempCoor.X - beginCoor.X) +
//                        (tempCoor.Y - beginCoor.Y) * (tempCoor.Y - beginCoor.Y);
            }
//            qDebug() << "calcEndingPos" << beginPos << endPos << powDis << fTempDis << fTempDis1;
            break;
        }
    }
    while(1);
    return insertedPos;
}

/**
 * @brief 顺时针反向计算路径终点位置并插入过渡点
 * 
 * @param path 输入输出参数,路径点集
 * @param beginPos 输入输出参数,起始位置
 * @param endPos 输入输出参数,结束位置
 * @param powDis 输入参数,距离阈值的平方
 * @return uint 返回插入点的位置,无插入则返回0xFFFFFFFF
 */
uint calcEndingPosInvert(Path &path, uint &beginPos, uint &endPos, const double &powDis)
{
    uint insertedPos = 0xFFFFFFFF;
    const auto beginCoor = path[beginPos];
    uint coorCnt = path.size();
    double fTempDis = 0.0;
//    double fTempDis1 = 0.0;

    // 向后搜索终点
    endPos = beginPos;
    do
    {
        ++ endPos;
        if(endPos > coorCnt - 1) endPos = 0;
        if(endPos == beginPos) {
            endPos = beginPos + 1;
            if(endPos > coorCnt - 1) endPos = 0;
            break;
        }
        fTempDis = (path[endPos].X - beginCoor.X) * (path[endPos].X - beginCoor.X) +
                (path[endPos].Y - beginCoor.Y) * (path[endPos].Y - beginCoor.Y);
        if(fTempDis > powDis) {
            double fMaxDis = 1.5 * powDis;
            if(powDis > 100 && (fTempDis > fMaxDis)) {
                uint prePos = endPos - 1;
                if(prePos > coorCnt - 1) prePos = coorCnt - 1;
                double fTempDisPre = (path[prePos].X - beginCoor.X) * (path[prePos].X - beginCoor.X) +
                        (path[prePos].Y - beginCoor.Y) * (path[prePos].Y - beginCoor.Y);
                double fRatio = sqrt((powDis - fTempDisPre) / (fTempDis - fTempDisPre));
                const auto tempCoor = IntPoint(qRound64(path[prePos].X + (path[endPos].X - path[prePos].X) * fRatio),
                                               qRound64(path[prePos].Y + (path[endPos].Y - path[prePos].Y) * fRatio));
                path.insert(path.begin() + int(prePos), tempCoor);
                insertedPos = prePos;
                ++ coorCnt;
                if(beginPos >= insertedPos) {
                    ++ beginPos;
                    if(beginPos > coorCnt - 1) beginPos = 0;
                }
                if(endPos >= insertedPos) {
                    ++ endPos;
                    if(endPos > coorCnt - 1) endPos = 0;
                }
//                fTempDis1 = (tempCoor.X - beginCoor.X) * (tempCoor.X - beginCoor.X) +
//                        (tempCoor.Y - beginCoor.Y) * (tempCoor.Y - beginCoor.Y);
            }
//            qDebug() << "calcEndingPosInvert" << beginPos << endPos << powDis << fTempDis << fTempDis1;
            break;
        }
    }
    while(1);
    return insertedPos;
}

/**
 * @brief 计算窗口信息(从起点向前)
 * 
 * @param path 输入输出参数,路径点集
 * @param wndInfo 输入输出参数,窗口信息
 * @param powDis 输入参数,距离阈值的平方
 * 
 * @details 处理流程:
 * 1. 计算第一段窗口:
 *    - 从起点向前搜索第一个终点
 * 
 * 2. 计算第二段窗口:
 *    - 从第一个终点继续向前搜索第二个终点
 *    - 处理插入点对起点位置的影响
 */
void calcWndInfo(Path &path, WndInfo &wndInfo, const double &powDis)
{
//    qDebug() << "calcWndInfo--0" << path.size() << wndInfo.nWndBegin << wndInfo.nWndEnd1 << wndInfo.nWndEnd2;
    calcEndingPos(path, wndInfo.nWndBegin, wndInfo.nWndEnd1, powDis);
//    qDebug() << "calcWndInfo--1" << path.size() << wndInfo.nWndBegin << wndInfo.nWndEnd1 << wndInfo.nWndEnd2;

    uint insertedPos = calcEndingPos(path, wndInfo.nWndEnd1, wndInfo.nWndEnd2, powDis);
    uint nTempSz = path.size();
    if(insertedPos < wndInfo.nWndBegin)
    {
        ++ wndInfo.nWndBegin;
        if(wndInfo.nWndBegin > nTempSz - 1) wndInfo.nWndBegin = 0;
    }
//    qDebug() << "calcWndInfo--2" << path.size() << wndInfo.nWndBegin << wndInfo.nWndEnd1 << wndInfo.nWndEnd2;
}

/**
 * @brief 计算窗口信息(从中点双向)
 * 
 * @param path 输入输出参数,路径点集
 * @param wndInfo 输入输出参数,窗口信息
 * @param powDis 输入参数,距离阈值的平方
 * 
 * @details 处理流程:
 * 1. 计算前半段窗口:
 *    - 从中点向后搜索起点
 * 
 * 2. 计算后半段窗口:
 *    - 从中点向前搜索终点
 *    - 处理插入点对起点位置的影响
 */
void calcWndInfoMid(Path &path, WndInfo &wndInfo, const double &powDis)
{
    calcEndingPosInvert(path, wndInfo.nWndEnd1, wndInfo.nWndBegin, powDis);
    uint insertedPos = calcEndingPos(path, wndInfo.nWndEnd1, wndInfo.nWndEnd2, powDis);
    uint nTempSz = path.size();
    if(insertedPos <= wndInfo.nWndBegin)
    {
        ++ wndInfo.nWndBegin;
        if(wndInfo.nWndBegin > nTempSz - 1) wndInfo.nWndBegin = 0;
    }
}

/**
 * @brief 更新子分支的窗口位置信息
 * 
 * @param insertedPos 输入参数,插入点位置
 * @param totalSz 输入参数,路径总大小
 * @param index 输入参数,当前处理的分支索引
 * @param subBranchList 输入输出参数,子分支列表
 * 维护路径点插入后的索引一致性，示例：
 * 原始路径: [0,1,2,3,4]
 * 窗口信息: begin=2, end1=4
 * 在位置1插入新点后:
 * 新路径: [0,1,NEW,2,3,4]
 * 更新窗口: begin=3, end1=5
 */
void updateSubBranch(const uint &insertedPos, const uint &totalSz, const int &index, QList<PocketBranch> &subBranchList)
{
    for(int iPos = 0; iPos < index; ++ iPos)
    {
        auto &wndIndo = subBranchList[iPos].mNodeList.first().ringWndInfo.bigWnd;
        if(wndIndo.nWndBegin >= insertedPos)
        {
            ++ wndIndo.nWndBegin;
            if(wndIndo.nWndBegin > totalSz) wndIndo.nWndBegin = 0;
        }
        if(wndIndo.nWndEnd1 >= insertedPos)
        {
            ++ wndIndo.nWndEnd1;
            if(wndIndo.nWndEnd1 > totalSz) wndIndo.nWndEnd1 = 0;
        }
    }
}

/**
 * @brief 处理整个分支结构并更新子分支
 * 
 * @param path 输入输出参数,路径点集
 * @param wndInfo 输入输出参数,窗口信息
 * @param powDis 输入参数,距离阈值的平方
 * @param index 输入参数,当前分支索引
 * @param subBranchList 输入输出参数,子分支列表
 * 
 * @details 只更新窗口的起点(nWndBegin)和第一终点(nWndEnd1)
 */
void calcBranchWndInfo(Path &path, WndInfo &wndInfo, const double &powDis, const int &index, QList<PocketBranch> &subBranchList)
{
    uint insertedPos = calcEndingPos(path, wndInfo.nWndBegin, wndInfo.nWndEnd1, powDis);
    updateSubBranch(insertedPos, path.size() - 1, index, subBranchList);
}

/**
 * @brief 处理单个窗口位置
 * 
 * @param path 输入输出参数,路径点集
 * @param wndPos 输入输出参数,窗口位置信息
 * @param powDis 输入参数,距离阈值的平方
 * 
 * @details 更新两段窗口的所有位置(beginPos_1, endPos_1, beginPos_2, endPos_2)，处理窗口段之间的连接关系
 */
void calcEndingPos(Path &path, WndPos &wndPos, const double &powDis)
{

    calcEndingPos(path, wndPos.beginPos_1, wndPos.endPos_1, powDis);

    wndPos.beginPos_2 = wndPos.endPos_1;

    uint insertedPos = calcEndingPos(path, wndPos.beginPos_2, wndPos.endPos_2, powDis);
    uint nTempSz = path.size();
    if(insertedPos <= wndPos.beginPos_1)
    {
        ++ wndPos.beginPos_1;
        if(wndPos.beginPos_1 > nTempSz - 1) wndPos.beginPos_1 = 0;
    }
    if(insertedPos <= wndPos.endPos_1)
    {
        ++ wndPos.endPos_1;
        if(wndPos.endPos_1 > nTempSz - 1) wndPos.endPos_1 = 0;
    }
}

/// @brief 正向复制指定段
/// @param des 输出参数,目标路径
/// @param src 输入参数,源路径 
/// @param begin 输入参数,起始位置
/// @param end 输入参数,结束位置
/// @details 从起点到终点依次复制点,处理循环索引
void copyCoor(Path &des, const Path &src, const uint &begin, const uint &end)
{
    uint coorCnt = src.size();
    if(begin > coorCnt - 1 || end > coorCnt - 1) return;

    uint iPos = begin;
    while(1)
    {
        des << src[iPos];
        if(end == iPos) break;

        ++ iPos;
        if(iPos > coorCnt - 1) iPos = 0;
    }
}

/// @brief 反向复制指定段
void copyCoorInvert(Path &des, const Path &src, const uint &begin, const uint &end)
{
    uint coorCnt = src.size();
    if(begin > coorCnt - 1 || end > coorCnt - 1) return;

    uint iPos = begin;
    while(1)
    {
        des << src[iPos];
        if(end == iPos) break;

        -- iPos;
        if(iPos > coorCnt - 1) iPos = coorCnt - 1;
    }
}

/// @brief 根据窗口信息复制特定段
void copyCoor(Path &des, const Path &src, const WndPos &wndPos, const int &copyType)
{
    switch(copyType)
    {
    case 0:
        copyCoor(des, src, wndPos.beginPos_1, wndPos.endPos_2);
        break;
    case 1:
        copyCoor(des, src, wndPos.beginPos_2, wndPos.endPos_1);
        break;
    }
}

/// 计算两个轮廓间的最近点并设置窗口位置
/// @param bigger 输入输出参数,大轮廓路径
/// @param biggerWndPos 输入输出参数,大轮廓窗口位置
/// @param smaller 输入参数,小轮廓路径
/// @param wndPos 输入输出参数,小轮廓窗口位置
/// @param begin 输入参数,搜索起始位置
/// @param end 输入参数,搜索结束位置
/// @param powDis 输入参数,距离阈值的平方
/// @param biggerWndList 输入输出参数,大轮廓窗口列表
/// @details 用于确定两个轮廓的最佳连接点,优化加工路径的连续性
void PocketBranch::calcClosedPt(Path &bigger, WndPos &biggerWndPos, 
                               Path &smaller, WndPos &wndPos,
                               const uint &begin, const uint &end, 
                               const double &powDis, 
                               QList<BiggerWnd> &biggerWndList)
{
    double fDisTemp = 1E300;
    uint nSmallSz = smaller.size();
    uint nBigSz = bigger.size();

    // 在指定范围内搜索最近点对
    for(uint iPos = begin; iPos < nBigSz;)
    {
        for(uint iSmall = 0; iSmall < nSmallSz; ++ iSmall)
        {
            // 计算两点间距离
            double fDis = pow((smaller[iSmall].X - bigger[iPos].X), 2) +
                    pow((smaller[iSmall].Y - bigger[iPos].Y), 2);
            // 更新最小距离和对应点位置
            if(fDis < fDisTemp)
            {
                fDisTemp = fDis;
                biggerWndPos.beginPos_1 = iPos;
                wndPos.beginPos_1 = iSmall;
            }
        }

        if(end == iPos) break;
        ++ iPos;
        if(iPos > nBigSz - 1) iPos = 0;
    }

    // 计算大轮廓的窗口终点
    uint insertedPos = calcEndingPos(bigger, biggerWndPos.beginPos_1, biggerWndPos.endPos_1, powDis);

    // 更新所有受影响的窗口位置
    nBigSz = bigger.size();
    for(auto &wnd : biggerWndList)
    {
        // 更新起点索引
        if(wnd.biggerWndPos.beginPos_1 >= insertedPos)
        {
            ++ wnd.biggerWndPos.beginPos_1;
            if(wnd.biggerWndPos.beginPos_1 > nBigSz - 1) wnd.biggerWndPos.beginPos_1 = 0;
        }
        // 更新终点索引
        if(wnd.biggerWndPos.endPos_1 >= insertedPos)
        {
            ++ wnd.biggerWndPos.endPos_1;
            if(wnd.biggerWndPos.endPos_1 > nBigSz - 1) wnd.biggerWndPos.endPos_1 = 0;
        }
    }
}


/// 反向计算两个轮廓间的最近点并设置窗口位置
/// @param bigger 输入输出参数,大轮廓路径
/// @param biggerWndPos 输入输出参数,大轮廓窗口位置
/// @param smaller 输入参数,小轮廓路径
/// @param wndPos 输入输出参数,小轮廓窗口位置
/// @param begin 输入参数,搜索起始位置
/// @param end 输入参数,搜索结束位置
/// @param powDis 输入参数,距离阈值的平方
/// @param biggerWndList 输入输出参数,大轮廓窗口列表
/// @details 用于反向搜索两个轮廓的最佳连接点,与calcClosedPt配合使用优化加工路径
void PocketBranch::calcClosedPtInvert(Path &bigger, WndPos &biggerWndPos, Path &smaller, WndPos &wndPos, const uint &begin, const uint &end, const double &powDis, QList<BiggerWnd> &biggerWndList)
{
    double fDisTemp = 1E300;
    uint nSmallSz = smaller.size();
    uint nBigSz = bigger.size();

    for(uint iPos = begin; iPos < nBigSz;)
    {
        for(uint iSmall = 0; iSmall < nSmallSz; ++ iSmall)
        {

            double fDis = pow((smaller[iSmall].X - bigger[iPos].X), 2) +
                    pow((smaller[iSmall].Y - bigger[iPos].Y), 2);
            if(fDis < fDisTemp)
            {
                fDisTemp = fDis;
                biggerWndPos.beginPos_1 = iPos;
                wndPos.beginPos_1 = iSmall;
            }
        }

        if(end == iPos) break;
        -- iPos;
        if(iPos > nBigSz - 1) iPos = nBigSz - 1;
    }
    uint insertedPos = calcEndingPos(bigger, biggerWndPos.beginPos_1, biggerWndPos.endPos_1, powDis);

    nBigSz = bigger.size();
    for(auto &wnd : biggerWndList)
    {
        if(wnd.biggerWndPos.beginPos_1 >= insertedPos)
        {
            ++ wnd.biggerWndPos.beginPos_1;
            if(wnd.biggerWndPos.beginPos_1 > nBigSz - 1) wnd.biggerWndPos.beginPos_1 = 0;
        }
        if(wnd.biggerWndPos.endPos_1 >= insertedPos)
        {
            ++ wnd.biggerWndPos.endPos_1;
            if(wnd.biggerWndPos.endPos_1 > nBigSz - 1) wnd.biggerWndPos.endPos_1 = 0;
        }
    }
}

/// 计算螺旋路径的连接点
/// @param curIndex 输入参数,当前节点索引
/// @param preIndex 输入参数,前一个节点索引
/// @param beginPos 输入输出参数,起始位置
/// @param endPos 输入输出参数,结束位置
/// @param powDis 输入参数,距离阈值的平方
/// @param maxCircle 输入参数,是否为最大轮廓
/// @details 实现步骤:
/// 1. 将前一段路径的终点添加到螺旋路径
/// 2. 计算当前轮廓上距离前一段终点最近的点作为新起点
/// 3. 计算新的终点位置
/// 4. 将计算出的这段路径添加到螺旋路径中
void PocketBranch::calcSpiralJoinPoint(const int &curIndex, int &preIndex, uint &beginPos, uint &endPos, const double &powDis, const bool &maxCircle)
{
//    double X1 = 0.0, Y1 = 0.0, X2 = 0.0, Y2 = 0.0;
//    double fK = 1.0;
//    double fDelta_X12 = 0.0, fDelta_Y12 = 0.0;
//    double fEndArea = 0.0;

    uint nCoorCnt = mNodeList[curIndex].circle.size();
    // 处理非最大轮廓的情况
    if(false == maxCircle)
    {
        if(nCoorCnt > 2)
        {   
            // 添加前一段路径的终点
            auto endCoor = mNodeList[preIndex].circle[endPos];
            spiralPath << endCoor;

//            auto beginCoor = mNodeList[preIndex].circle[beginPos];
//            X1 = (beginCoor.X + endCoor.X) * 0.5;
//            Y1 = (beginCoor.Y + endCoor.Y) * 0.5;
//            if(beginCoor.X == endCoor.X)
//            {
//                Y2 = Y1;
//                X2 = X1 + 100000;
//                if(beginCoor.Y == endCoor.Y) {
////                    continue;
//                }
//            }
//            else if(beginCoor.Y == endCoor.Y)
//            {
//                X2 = X1;
//                Y2 = Y1 + 100000;
//            }
//            else
//            {
//                fK =  - double(endCoor.X - beginCoor.X) / (endCoor.Y - beginCoor.Y);
//                X2 = X1 + 100000;
//                Y2 = 100000 * fK + Y1;
//            }
//            fDelta_X12 = X1 - X2;
//            fDelta_Y12 = Y1 - Y2;
//            fEndArea = fDelta_X12 * (endCoor.Y - Y2) - fDelta_Y12 * (endCoor.X - X2);

            // 计算距离前一段终点最近的点作为新起点
            double fDisTemp = 1E300;
            for(uint iPos = 0; iPos < nCoorCnt; ++ iPos)
            {
                double fDis = pow((endCoor.X - mNodeList[curIndex].circle[iPos].X), 2) +
                        pow((endCoor.Y - mNodeList[curIndex].circle[iPos].Y), 2);
                if(fDis < fDisTemp)
                {
                    fDisTemp = fDis;
                    beginPos = iPos;
                }
            }

            // 处理索引越界
            if(beginPos > nCoorCnt - 1) beginPos -= nCoorCnt;
            // 计算新的终点位置
            calcEndingPos(mNodeList[curIndex].circle, beginPos, endPos, powDis);
        }
        else
        {
            beginPos = 0;
            endPos = 0;
        }
    }
    // 将这段路径添加到螺旋路径
    copyCoor(spiralPath, mNodeList[curIndex].circle, beginPos, endPos);
}

/// 创建螺旋路径
/// @param powDis 输入参数,距离阈值的平方,用于控制路径点间距
/// @details 实现步骤:
/// 1. 单节点情况:直接复制轮廓并闭合
/// 2. 多节点情况:按窗口信息复制每个节点的部分轮廓
/// 3. 递归处理所有子分支
void PocketBranch::createSpiral(const double &powDis)
{
    int nNodeCnt = mNodeList.size();

    // 处理单节点情况
    if(1 == nNodeCnt)
    {
        // 复制完整轮廓并闭合
        for(const auto &coor : mNodeList.first().circle)
        {
            spiralPath << coor;
        }
        spiralPath << mNodeList.first().circle.front();
    }
    // 处理多节点情况
    else if(nNodeCnt > 1)
    {
        // 按窗口信息复制每个节点的部分轮廓
        for(int iIndex = 0; iIndex < nNodeCnt; ++ iIndex)
        {
            const auto &WndInfo = mNodeList[iIndex].ringWndInfo.curWnd;
            copyCoor(spiralPath, mNodeList[iIndex].circle, WndInfo.nWndBegin, WndInfo.nWndEnd1);
        }
    }

    // 递归处理子分支
    for(auto &branch : mSubBranchList)
    {
        branch.createSpiral(powDis);
    }
}

/// 创建费马螺旋路径
/// @param powDis 输入参数,距离阈值的平方
/// @details 实现步骤:
/// 1. 有子分支的情况:
///    - 先处理主轮廓路径
///    - 递归处理所有子分支
///    - 按层级顺序连接路径
/// 2. 无子分支的情况:
///    - 直接处理主轮廓路径
///    - 按层级顺序连接
void PocketBranch::createFermatSpiral(const double &powDis)
{
    // 有子分支的情况
    if(mSubBranchList.size())
    {
        Path path;
        int nNodeSz = mNodeList.size();
        IntPoint lastCoor;

        // 处理主轮廓路径
        if(true/*nNodeSz > 1*/)
        {
            // 第一次遍历:从外到内
            for(int iNode = 0; iNode < nNodeSz - 1; ++ iNode)
            {
                // 根据层级和类型选择复制方向
                WndInfo &wndInfo = mNodeList[iNode].ringWndInfo.curWnd;
                if(bool(mNodeList[iNode].nLevel & 0x1) == bool(mCircleType))
                {
                    // 反向复制路径
                    if(0 == iNode) copyCoorInvert(path, mNodeList[iNode].circle, mNodeList[iNode].ringWndInfo.tempWnd.nWndEnd1, wndInfo.nWndEnd1);
                    else path << mNodeList[iNode].circle[wndInfo.nWndEnd1];
                }
                else
                // 正向复制路径
                    copyCoor(path, mNodeList[iNode].circle,
                             0 == iNode ? mNodeList[iNode].ringWndInfo.tempWnd.nWndBegin : wndInfo.nWndBegin,
                             (nNodeSz - 1 == iNode) ? wndInfo.nWndEnd1 : wndInfo.nWndEnd2);
            }
            // 处理子分支
            drawPathOffset(path, IntPoint(25000, 50000), 10);

            if(path.size()) lastCoor = path.back();
            path.clear();
            auto &curNode = mNodeList.last();
            if(bool(curNode.nLevel & 0x1) == bool(mCircleType))
            {
                uint nBeginPos = curNode.ringWndInfo.curWnd.nWndEnd1;
                uint iBegin = curNode.ringWndInfo.curWnd.nWndEnd1;
                uint nEnd = curNode.ringWndInfo.curWnd.nWndBegin;

                QList<uint> listBeginPos;
                for(auto &branch : mSubBranchList)
                {
                    listBeginPos << branch.mNodeList.first().ringWndInfo.bigWnd.nWndBegin;
                }
                int nIndex = -1;
                while(true)
                {
                    nIndex = listBeginPos.indexOf(iBegin);

                    if(-1 != nIndex)
                    {
                        path << lastCoor;
                        auto &branch = mSubBranchList[nIndex];
                        copyCoorInvert(path, curNode.circle, nBeginPos, branch.mNodeList.first().ringWndInfo.bigWnd.nWndBegin);
                        nBeginPos = branch.mNodeList.first().ringWndInfo.bigWnd.nWndEnd1;
                        path << branch.mNodeList.first().circle[branch.mNodeList.first().ringWndInfo.tempWnd.nWndBegin];
                        drawPathOffset(path, IntPoint(25000, 50000), 10);
                        path.clear();
                        branch.createFermatSpiral(powDis);
                        lastCoor = branch.mNodeList.first().circle[branch.mNodeList.first().ringWndInfo.tempWnd.nWndEnd1];
                    }
                    if(iBegin == nEnd) break;

                    -- iBegin;
                    if(iBegin > curNode.circle.size() - 1) iBegin = curNode.circle.size() - 1;
                }

                path << lastCoor;
                copyCoorInvert(path, curNode.circle, nBeginPos, curNode.ringWndInfo.curWnd.nWndBegin);
                drawPathOffset(path, IntPoint(25000, 50000), 10);
                path.clear();
            }
            else
            {
                uint nBeginPos = curNode.ringWndInfo.curWnd.nWndBegin;
                uint iBegin = curNode.ringWndInfo.curWnd.nWndBegin;
                uint nEnd = curNode.ringWndInfo.curWnd.nWndEnd1;

                std::sort(mSubBranchList.begin(), mSubBranchList.end(), compareTotalBranchWnd);
                QList<uint> listBeginPos;
                for(auto &branch : mSubBranchList)
                {
                    listBeginPos << branch.mNodeList.first().ringWndInfo.bigWnd.nWndEnd1;
                }
                int nIndex = -1;

                while(true)
                {
                    nIndex = listBeginPos.indexOf(iBegin);

                    if(-1 != nIndex)
                    {
                        path << lastCoor;
                        auto &branch = mSubBranchList[nIndex];
                        copyCoor(path, curNode.circle, nBeginPos, branch.mNodeList.first().ringWndInfo.bigWnd.nWndEnd1);
                        nBeginPos = branch.mNodeList.first().ringWndInfo.bigWnd.nWndBegin;
                        path << branch.mNodeList.first().circle[branch.mNodeList.first().ringWndInfo.tempWnd.nWndEnd1];
                        drawPathOffset(path, IntPoint(25000, 50000), 10);
                        path.clear();
                        branch.createFermatSpiral(powDis);
                        lastCoor = branch.mNodeList.first().circle[branch.mNodeList.first().ringWndInfo.tempWnd.nWndBegin];
                    }
                    if(iBegin == nEnd) break;

                    ++ iBegin;
                    if(iBegin > curNode.circle.size() - 1) iBegin = 0;
                }
                path << lastCoor;
                copyCoor(path, curNode.circle, nBeginPos, curNode.ringWndInfo.curWnd.nWndEnd1);
                drawPathOffset(path, IntPoint(25000, 50000), 10);
                path.clear();
            }

            path.clear();
            // 第二次遍历:从内到外
            for(int iNode = nNodeSz - 2; iNode >= 0; -- iNode)
            {
                // 根据层级和类型选择复制方向
                WndInfo &wndInfo = mNodeList[iNode].ringWndInfo.curWnd;

                if(bool(mNodeList[iNode].nLevel & 0x1) == bool(mCircleType)) {
                    if(0 == iNode)
                        copyCoorInvert(path, mNodeList[iNode].circle, wndInfo.nWndEnd2,
                                       mNodeList[iNode].ringWndInfo.tempWnd.nWndBegin);
                    else
                        copyCoorInvert(path, mNodeList[iNode].circle, wndInfo.nWndEnd2, wndInfo.nWndBegin);
                }
                else {
                    path << mNodeList[iNode].circle[wndInfo.nWndEnd1];
                    if(0 == iNode)
                    {
                        copyCoor(path, mNodeList[iNode].circle, wndInfo.nWndEnd1,
                                       mNodeList[iNode].ringWndInfo.tempWnd.nWndEnd1);
                    }
                }
            }
            drawPathOffset(path, IntPoint(25000, 50000), 10);
        }
        else if(1 == nNodeSz)
        {
            bool bFirstLoop = true;
            auto &curNode = mNodeList.last();
            if(bool(curNode.nLevel & 0x1) == bool(mCircleType))
            {
                uint nBeginPos = curNode.ringWndInfo.tempWnd.nWndEnd1;
                uint iBegin = curNode.ringWndInfo.tempWnd.nWndEnd1;
                uint nEnd = curNode.ringWndInfo.tempWnd.nWndBegin;

                QList<uint> listBeginPos;
                for(auto &branch : mSubBranchList)
                {
                    listBeginPos << branch.mNodeList.first().ringWndInfo.bigWnd.nWndBegin;
                }
                int nIndex = -1;
                while(true)
                {
                    nIndex = listBeginPos.indexOf(iBegin);

                    if(-1 != nIndex)
                    {
                        if(!bFirstLoop) path << lastCoor;
                        bFirstLoop = false;
                        auto &branch = mSubBranchList[nIndex];
                        copyCoorInvert(path, curNode.circle, nBeginPos, branch.mNodeList.first().ringWndInfo.bigWnd.nWndBegin);
                        nBeginPos = branch.mNodeList.first().ringWndInfo.bigWnd.nWndEnd1;
                        path << branch.mNodeList.first().circle[branch.mNodeList.first().ringWndInfo.tempWnd.nWndBegin];
                        drawPathOffset(path, IntPoint(25000, 50000), 10);
                        path.clear();
                        branch.createFermatSpiral(powDis);
                        lastCoor = branch.mNodeList.first().circle[branch.mNodeList.first().ringWndInfo.tempWnd.nWndEnd1];
                    }
                    if(iBegin == nEnd) break;

                    -- iBegin;
                    if(iBegin > curNode.circle.size() - 1) iBegin = curNode.circle.size() - 1;
                }

                path << lastCoor;
                copyCoorInvert(path, curNode.circle, nBeginPos, curNode.ringWndInfo.curWnd.nWndBegin);
                drawPathOffset(path, IntPoint(25000, 50000), 10);
                path.clear();
            }
            else
            {
                uint nBeginPos = curNode.ringWndInfo.tempWnd.nWndBegin;
                uint iBegin = curNode.ringWndInfo.tempWnd.nWndBegin;
                uint nEnd = curNode.ringWndInfo.tempWnd.nWndEnd1;

                std::sort(mSubBranchList.begin(), mSubBranchList.end(), compareTotalBranchWnd);
                QList<uint> listBeginPos;
                for(auto &branch : mSubBranchList)
                {
                    listBeginPos << branch.mNodeList.first().ringWndInfo.bigWnd.nWndEnd1;
                }
                int nIndex = -1;

                while(true)
                {
                    nIndex = listBeginPos.indexOf(iBegin);

                    if(-1 != nIndex)
                    {
                        if(!bFirstLoop) path << lastCoor;
                        bFirstLoop = false;
                        auto &branch = mSubBranchList[nIndex];
                        copyCoor(path, curNode.circle, nBeginPos, branch.mNodeList.first().ringWndInfo.bigWnd.nWndEnd1);
                        nBeginPos = branch.mNodeList.first().ringWndInfo.bigWnd.nWndBegin;
                        path << branch.mNodeList.first().circle[branch.mNodeList.first().ringWndInfo.tempWnd.nWndEnd1];
                        drawPathOffset(path, IntPoint(25000, 50000), 10);
                        path.clear();
                        branch.createFermatSpiral(powDis);
                        lastCoor = branch.mNodeList.first().circle[branch.mNodeList.first().ringWndInfo.tempWnd.nWndBegin];
                    }
                    if(iBegin == nEnd) break;

                    ++ iBegin;
                    if(iBegin > curNode.circle.size() - 1) iBegin = 0;
                }

                path << lastCoor;
                copyCoor(path, curNode.circle, nBeginPos, curNode.ringWndInfo.curWnd.nWndEnd1);
                drawPathOffset(path, IntPoint(25000, 50000), 10);
                path.clear();
            }
        }
    }
    // 无子分支的情况
    else
    {
        int nNodeSz = mNodeList.size();
        Path path;
        for(int iNode = 0; iNode < nNodeSz; ++ iNode)
        {
            WndInfo &wndInfo = mNodeList[iNode].ringWndInfo.curWnd;

            if(bool(mNodeList[iNode].nLevel & 0x1) == bool(mCircleType))
            {
                if(0 == iNode) copyCoorInvert(path, mNodeList[iNode].circle, mNodeList[iNode].ringWndInfo.tempWnd.nWndEnd1, wndInfo.nWndEnd1);
                else path << mNodeList[iNode].circle[wndInfo.nWndEnd1];
            }
            else
                copyCoor(path, mNodeList[iNode].circle,
                         0 == iNode ? mNodeList[iNode].ringWndInfo.tempWnd.nWndBegin : wndInfo.nWndBegin,
                         (nNodeSz - 1 == iNode) ? wndInfo.nWndEnd1 : wndInfo.nWndEnd2);
        }

        for(int iNode = nNodeSz - 1; iNode >= 0; -- iNode)
        {
            WndInfo &wndInfo = mNodeList[iNode].ringWndInfo.curWnd;

            if(bool(mNodeList[iNode].nLevel & 0x1) == bool(mCircleType)) {
                if(0 == iNode)
                    copyCoorInvert(path, mNodeList[iNode].circle, wndInfo.nWndEnd2,
                                   mNodeList[iNode].ringWndInfo.tempWnd.nWndBegin);
                else
                    copyCoorInvert(path, mNodeList[iNode].circle, wndInfo.nWndEnd2, wndInfo.nWndBegin);
            }
            else {
                path << mNodeList[iNode].circle[wndInfo.nWndEnd1];
                if(0 == iNode)
                {
                    copyCoor(path, mNodeList[iNode].circle, wndInfo.nWndEnd1,
                                   mNodeList[iNode].ringWndInfo.tempWnd.nWndEnd1);
                }
            }
        }
        drawPathOffset(path, IntPoint(25000, 50000), 10);
    }
}

/// 计算螺旋路径的节点窗口位置
/// @param powDis 输入参数,距离阈值的平方
/// @details 实现步骤:
/// 1. 多节点情况:从内到外计算相邻节点间的窗口位置
/// 2. 单节点情况:设置窗口为整个轮廓
/// 3. 递归处理所有子分支
void PocketBranch::calcNodeWndPos_Spiral(const double &powDis)
{
    int nNodeSz = mNodeList.size();
    
    // 处理多节点情况
    if(nNodeSz > 1)
    {
        // 从内到外计算相邻节点的窗口位置
        for(int iNode = nNodeSz - 1; iNode > 0; -- iNode)
        {
            calcNodeWndPos(mNodeList[iNode], mNodeList[iNode - 1], powDis, iNode == nNodeSz - 1);
        }
    }
    // 处理单节点情况
    else if(1 == nNodeSz)
    {
        // 设置窗口为整个轮廓
        auto &wndInfo = mNodeList.first().ringWndInfo.curWnd;
        wndInfo.nWndBegin = 0;
        wndInfo.nWndEnd1 = mNodeList.first().circle.size() - 1;
    }

    // 递归处理子分支
    for(auto &branch : mSubBranchList)
    {
        branch.calcNodeWndPos_Spiral(powDis);
    }
}


/// 计算节点窗口位置
/// @param powDis 输入参数,距离阈值的平方
/// @details 实现步骤:
/// 1. 多节点情况:从内到外计算相邻节点间的窗口位置
/// 2. 单节点情况:直接计算窗口信息
/// 3. 处理子分支的窗口位置
/// 4. 递归处理所有子分支
/// 5. 处理首节点的临时窗口
/// 这种处理方式更适合复杂的分支结构
void PocketBranch::calcNodeWndPos(const double &powDis)
{
    int nNodeSz = mNodeList.size();
    
    // 处理多节点情况
    if(nNodeSz > 1)
    {
        // 从内到外计算相邻节点的窗口位置
        for(int iNode = nNodeSz - 1; iNode > 0; -- iNode)
        {
            calcNodeWndPos(mNodeList[iNode], mNodeList[iNode - 1], powDis, iNode == nNodeSz - 1);
        }
    }
    // 处理单节点情况
    else if(1 == nNodeSz)
    {
        // 直接计算窗口信息
        calcWndInfo(mNodeList.first().circle, mNodeList.first().ringWndInfo.curWnd, powDis);
    }

    // 处理子分支的窗口位置
    int nSubBranchCnt = mSubBranchList.size();
    for(int iBranch = 0; iBranch < nSubBranchCnt; ++ iBranch)
    {
        calcBranchWndPos(mSubBranchList[iBranch].mNodeList.first(), mNodeList.last(), powDis, iBranch);
    }

    // 递归处理子分支
    for(auto &branch : mSubBranchList)
    {
        branch.calcNodeWndPos(powDis);
    }

    // 处理首节点的临时窗口
    if(mNodeList.first().ringWndInfo.tempWnd.nWndBegin == mNodeList.first().ringWndInfo.tempWnd.nWndEnd1)
    {
        mNodeList.first().ringWndInfo.tempWnd.nWndBegin = mNodeList.first().ringWndInfo.curWnd.nWndBegin;
        mNodeList.first().ringWndInfo.tempWnd.nWndEnd1 = mNodeList.first().ringWndInfo.curWnd.nWndBegin;
    }
}


/// 计算节点间的窗口位置
/// @param curNode 输入参数,当前节点
/// @param biggerNode 输入参数,外层节点
/// @param powDis 输入参数,距离阈值的平方
/// @param minCircle 输入参数,是否为最内层圆
/// @details 实现步骤:
/// 1. 最内层圆的处理:
///    - 计算外层轮廓线段长度
///    - 寻找最佳连接点和插入点
///    - 计算两个节点的窗口信息
/// 2. 普通圆的处理:
///    - 找到距离当前起点最近的外层点
///    - 计算外层节点的窗口信息
void PocketBranch::calcNodeWndPos(PocketNode &curNode, PocketNode &biggerNode, 
                                 const double &powDis, const bool &minCircle)
{
    Path &smaller = curNode.circle;
    Path &bigger = biggerNode.circle;
    
    double fMinDis = 1E300;
    uint nBigPos = 0;
    uint nBeginPos = 0;
    qint64 nTempX = 0.0;
    qint64 nTempY = 0.0;
    
    uint nBigSz = bigger.size();
    uint nSmallSz = smaller.size();

    // 处理最内层圆
    if(minCircle)
    {
        // 计算外层轮廓每段线段的长度
        double *fDisBigger = new double[nBigSz];
        for(uint iPos = 0, jPos = nBigSz - 1; iPos < nBigSz; ++ iPos)
        {
            fDisBigger[iPos] = sqrt((pow((bigger[jPos].X - bigger[iPos].X), 2) +
                                    pow((bigger[jPos].Y - bigger[iPos].Y), 2)));
            jPos = iPos;
        }

        // 寻找最佳连接点和插入点
        // 通过计算垂直距离找到最佳连接位置
        // 计算插入点的精确坐标
        
        // 设置内层圆的窗口信息
        curNode.ringWndInfo.curWnd.nWndBegin = nBeginPos;
        calcWndInfo(smaller, curNode.ringWndInfo.curWnd, powDis);

        // 在外层插入连接点并设置窗口信息
        bigger.insert(bigger.begin() + int(nBigPos), IntPoint(nTempX, nTempY));
        auto &wndInfo = biggerNode.ringWndInfo.curWnd;
        wndInfo.nWndBegin = nBigPos;
        calcEndingPos(bigger, wndInfo.nWndBegin, wndInfo.nWndEnd1, powDis);
    }
    // 处理普通圆
    else
    {
        // 找到距离当前起点最近的外层点
        uint nSamllIndex = curNode.ringWndInfo.curWnd.nWndBegin;
        double fX1 = smaller[nSamllIndex].X;
        double fY1 = smaller[nSamllIndex].Y;
        
        for(uint iPos = 0; iPos < nBigSz; ++ iPos)
        {
            double fDis = (pow((fX1 - bigger[iPos].X), 2) +
                          pow((fY1 - bigger[iPos].Y), 2));
            if(fDis < fMinDis)
            {
                fMinDis = fDis;
                nBigPos = iPos;
            }
        }

        // 设置外层圆的窗口信息
        auto &wndInfo = biggerNode.ringWndInfo.curWnd;
        wndInfo.nWndBegin = nBigPos;
        calcEndingPos(bigger, wndInfo.nWndBegin, wndInfo.nWndEnd1, powDis);
    }
}

/// 计算分支节点的窗口位置
/// @param curNode 输入参数,当前分支节点
/// @param biggerNode 输入参数,主轮廓节点
/// @param powDis 输入参数,距离阈值的平方
/// @param index 输入参数,分支索引
/// @details 实现步骤:
/// 1. 计算主轮廓线段长度
/// 2. 寻找最佳连接点和插入点
/// 3. 更新分支节点的窗口信息
/// 4. 更新主轮廓的窗口信息
void PocketBranch::calcBranchWndPos(PocketNode &curNode, PocketNode &biggerNode, const double &powDis, const int &index)
{
    Path &smaller = curNode.circle;     // 分支轮廓
    Path &bigger = biggerNode.circle;   // 主轮廓

    // 初始化变量
    double fMinDis = 1E300;
    uint nBigPos = 0;
    uint nBeginPos = 0;
    qint64 nTempX = 0.0;
    qint64 nTempY = 0.0;

    uint nBigSz = bigger.size();
    uint nSmallSz = smaller.size();

    // 1. 计算主轮廓每段线段的长度
    double *fDisBigger = new double[nBigSz];
    for(uint iPos = 0, jPos = nBigSz - 1; iPos < nBigSz; ++ iPos)
    {
        fDisBigger[iPos] = sqrt((pow((bigger[jPos].X - bigger[iPos].X), 2) +
                                pow((bigger[jPos].Y - bigger[iPos].Y), 2)));
        jPos = iPos;
    }

    // 2. 寻找最佳连接点和插入点
    // 通过计算垂直距离找到最佳连接位置
    double fK = 1.0;
    double fDelta_X12 = 0.0, fDelta_Y12 = 0.0;
    double fSmall_X12 = 0.0, fSmall_Y12 = 0.0;
    double fKTemp = 1.0, fB1 = 0.0, fB2 = 0.0;
    double fX1 = 0.0, fY1 = 0.0;
    double fX2 = 0.0, fY2 = 0.0;
    qint64 fBigX1 = 0.0, fBigY1 = 0.0;
    qint64 fBigX2 = 0.0, fBigY2 = 0.0;

    // 计算插入点的精确坐标
    for(uint iPos = 0, jPos = nBigSz - 1; iPos < nBigSz; ++ iPos)
    {
        fBigX1 = bigger[iPos].X;
        fBigY1 = bigger[iPos].Y;

        fBigX2 = bigger[jPos].X;
        fBigY2 = bigger[jPos].Y;

        fDelta_X12 = fBigX2 - fBigX1;
        fDelta_Y12 = fBigY2 - fBigY1;

        if(fBigX2 != fBigX1 && fBigY2 != fBigY1)
        {
            fK = - fDelta_X12 / fDelta_Y12;
        }

        for(uint iSmall = 0; iSmall < nSmallSz; ++ iSmall)
        {
            fX1 = smaller[iSmall].X;
            fY1 = smaller[iSmall].Y;
            if(fBigX2 == fBigX1)
            {
                fX2 = fX1 + 100000;
                fY2 = fY1;
            }
            else if(fBigY2 == fBigY1)
            {
                fX2 = fX1;
                fY2 = fY1 + 100000;
            }
            else
            {
                fX2 = fX1 + 100000;
                fY2 = 100000 * fK + fY1;
            }
            fSmall_X12 = fX1 - fX2;
            fSmall_Y12 = fY1 - fY2;
            if((fSmall_X12 * (fBigY1 - fY2) - fSmall_Y12 * (fBigX1 - fX2)) *
                    (fSmall_X12 * (fBigY2 - fY2) - fSmall_Y12 * (fBigX2 - fX2)) > 0) continue;

            double fDis = fabs(fDelta_X12 * (bigger[iPos].Y - smaller[iSmall].Y) -
                               fDelta_Y12 * (bigger[iPos].X - smaller[iSmall].X)) / fDisBigger[iPos];
            if(fDis < fMinDis)
            {
                fMinDis = fDis;
                nBeginPos = iSmall;
                nBigPos = iPos;

                fKTemp = fK;
                fB1 = fY1 - fK * fX1;
                fB2 = fBigY1 + fBigX1 / fK;

                nTempX = qRound64((fB2 - fB1) / (fK + 1 / fK));
                nTempY = qRound64(fK * nTempX + fB1);
            }
        }
        jPos = iPos;
    }
    qDebug() << "Branch fMinDis----0-0" << fMinDis;
    delete []fDisBigger;
    // 3. 更新分支节点的窗口信息
    curNode.ringWndInfo.tempWnd.nWndBegin = nBeginPos;
    uint insertedPos = calcEndingPos(smaller, curNode.ringWndInfo.tempWnd.nWndBegin, curNode.ringWndInfo.tempWnd.nWndEnd1, powDis);

    // 处理窗口位置的索引更新
    uint nPathSz = smaller.size() - 1;
    // 更新起点位置
    if(curNode.ringWndInfo.curWnd.nWndBegin >= insertedPos)
    {
        ++ curNode.ringWndInfo.curWnd.nWndBegin;
        if(curNode.ringWndInfo.curWnd.nWndBegin > nPathSz) curNode.ringWndInfo.curWnd.nWndBegin = 0;
    }
    // 更新终点1位置
    if(curNode.ringWndInfo.curWnd.nWndEnd1 >= insertedPos)
    {
        ++ curNode.ringWndInfo.curWnd.nWndEnd1;
        if(curNode.ringWndInfo.curWnd.nWndEnd1 > nPathSz) curNode.ringWndInfo.curWnd.nWndEnd1 = 0;
    }
    // 更新终点2位置
    if(curNode.ringWndInfo.curWnd.nWndEnd2 >= insertedPos)
    {
        ++ curNode.ringWndInfo.curWnd.nWndEnd2;
        if(curNode.ringWndInfo.curWnd.nWndEnd2 > nPathSz) curNode.ringWndInfo.curWnd.nWndEnd2 = 0;
    }
    // 4. 更新主轮廓的窗口信息
    bigger.insert(bigger.begin() + int(nBigPos), IntPoint(nTempX, nTempY));
    updateSubBranch(nBigPos, bigger.size() - 1, index, mSubBranchList);
    curNode.ringWndInfo.bigWnd.nWndBegin = nBigPos;
    calcBranchWndInfo(bigger, curNode.ringWndInfo.bigWnd, powDis, index, mSubBranchList);
}

//PocketTree
// 简单追加分支到列表末尾
void PocketTree::appendBranch(const PocketBranch &branch)
{
    branchList << branch;
}

// 递归计算所有分支的节点总数
int PocketTree::getNodeCnt(int &nodeCnt)
{
    nodeCnt = 0;
    int nBranchCnt = branchList.size();
    for(const auto &branch : branchList)
    {
        branch.getNodeCnt(nodeCnt);
    }
    return nBranchCnt;
}

// 更新所有分支的顺序
void PocketTree::updateBranchOrder()
{
    for(auto &branch : branchList)
    {
        branch.updateBranchOrder();
    }
}

// 在指定索引位置插入分支
void InnerPocketTree::appendBranch(const PocketBranch &branch)
{
    branchList.insert(nIndex ++, branch);
}

// 递归计算所有分支的节点总数
int InnerPocketTree::getNodeCnt(int &nodeCnt)
{
    nodeCnt = 0;
    int nBranchCnt = branchList.size();
    for(const auto &branch : branchList) {
        branch.getNodeCnt(nodeCnt);
    }
    return nBranchCnt;
}

// 更新分支间的嵌套关系
void InnerPocketTree::updateNestingList()
{
    // 清除现有嵌套关系
    for(auto &branch : branchList) {
        branch.mNestingList.clear();
    }
    // 遍历所有分支对,更新嵌套关系
    auto keys = branchList.keys();
    int nCnt = keys.size();
    for(int iCnt = 0; iCnt < nCnt - 1; ++ iCnt) {
        int key = keys[iCnt];
        auto &branch = branchList[key];
        for(int jCnt = iCnt + 1; jCnt < nCnt; ++ jCnt) {
            int subKey = keys[jCnt];
            auto &subBranch = branchList[subKey];
            branch.updateNestingList(key, subBranch, subKey);
        }
    }
}

// 移除嵌套列表中的子分支
void InnerPocketTree::removeNestingList(QList<int> &subBranchList)
{
    // 创建临时列表存储结果
    QList<int> tempList = subBranchList;

    // 遍历所有输入的分支键值
    for(const auto &key : subBranchList) {
        // 获取当前分支
        auto &branch = branchList[key];
        
        // 检查当前分支的嵌套列表
        for(const auto &nestingKey : branch.mNestingList) {
            // 如果临时列表包含被嵌套的分支,则移除
            if(tempList.contains(nestingKey)) 
                tempList.removeAll(nestingKey);
        }
    }

    // 更新输入列表为处理后的结果
    subBranchList = tempList;
}

/// 创建口袋树结构
/// @param curPaths 输入参数,当前路径集合
/// @param outerTree 输出参数,外部口袋树
/// @param innerTree 输出参数,内部口袋树
/// @details 实现步骤:
/// 1. 遍历所有路径,根据面积正负分类处理
/// 2. 正面积创建外部分支
/// 3. 负面积创建内部分支
/// 4. 对外部分支排序
/// 5. 更新内部分支嵌套关系
void AlgorithmHatchingRing::createPocketTree(const Paths &curPaths, 
                                           PocketTree &outerTree, 
                                           InnerPocketTree &innerTree)
{
    // 遍历所有路径
    for(const auto &path : curPaths)
    {
        if(path.size() < 2) continue;
        BoundingBox bBox;
        double fArea = Area(path, bBox);

        // 处理正面积路径(外部轮廓)
        if(fArea >= 0)
        {
            PocketNode node(0, fArea, bBox, path);
            PocketBranch branch(PocketBranch::Outer, node);
            outerTree.appendBranch(branch);
        }
        // 处理负面积路径(内部轮廓)
        else
        {
            auto tempPath = path;
            ReversePath(tempPath);  // 反转路径方向
            PocketNode node(0, fArea, bBox, tempPath);
            PocketBranch branch(PocketBranch::Inner, node);
            innerTree.appendBranch(branch);
        }
    }

    // 对外部分支按面积排序
    std::sort(outerTree.branchList.begin(), outerTree.branchList.end(), compareLessThan);
    
    // 更新内部分支的嵌套关系
    innerTree.updateNestingList();
}


/// 更新口袋树结构
/// @param curPaths 输入参数,当前路径集合
/// @param outerTree 输出参数,外部口袋树
/// @param innerTree 输出参数,内部口袋树
/// @param level 输入参数,当前层级
void AlgorithmHatchingRing::updatePocketTree(const Paths &curPaths, 
                                           PocketTree &outerTree, 
                                           InnerPocketTree &innerTree, 
                                           const int &level)
{
    uint nPathCnt = curPaths.size();
    // 临时存储新的节点
    QList<PocketNode> tempOutterNodeList;
    QList<PocketNode> tempinnerNodeList;

    // 第一阶段:分类处理路径
    for(uint iPath = 0; iPath < nPathCnt; ++ iPath)
    {
        auto &path = curPaths[iPath];
        if(path.size() < 2) continue;
        BoundingBox bBox;
        double fArea = Area(path, bBox);
        // 处理外部节点(正面积)
        if(fArea >= 0)
        {
            PocketNode node(level, fArea, bBox, path);
            const auto coor = node.firstCoor();
            int nBranchIndex = 0;
            // 找到包含该节点的分支
            for(auto &branch : outerTree.branchList)
            {
                ++ nBranchIndex;
                if(branch.containsNode(coor, tempOutterNodeList.size())) break;
            }
            tempOutterNodeList << node;
        }
        // 处理内部节点(负面积)
        else
        {
            auto tempPath = path;
            ReversePath(tempPath);
            PocketNode node(level, fArea, bBox, tempPath);
            auto keys = innerTree.branchList.keys();
            // 检查所有内部分支是否包含该节点
            for(const auto &key : keys)
            {
                const auto &branch = innerTree.branchList[key];
                branch.containsInNode(node, key);
            }
            tempinnerNodeList << node;
        }
    }

    // 第二阶段:更新外部树
    for(auto &branch : outerTree.branchList)
    {
        branch.addNode(tempOutterNodeList);
    }

    // 第三阶段:更新内部树
    bool bBranchChanged = false;
    for(auto &node : tempinnerNodeList)
    {
        // 移除嵌套关系
        innerTree.removeNestingList(node.subBranchList);
        
        // 单一分支情况
        if(1 == node.subBranchList.size())
        {
            innerTree.branchList[node.subBranchList.at(0)].mNodeList.insert(0, node);
        }
        // 多分支情况
        else if(node.subBranchList.size() > 1) 
        {
            bBranchChanged = true;
            // 创建新分支并合并现有分支
            PocketBranch branch(PocketBranch::Inner, node);
            for(const auto &key : node.subBranchList)
            {
                branch.mSubBranchList << innerTree.branchList[key];
                innerTree.branchList.remove(key);
            }
            innerTree.appendBranch(branch);
        }
    }

    // 如果分支结构改变,更新嵌套关系
    if(bBranchChanged) 
        innerTree.updateNestingList();
}

/// 排序口袋树
void AlgorithmHatchingRing::sortPocketTree(PocketTree &outerTree, InnerPocketTree &innerTree)
{
    mergePocketTree(outerTree, innerTree);
    outerTree.updateBranchOrder(); 
}

/// 创建普通螺旋路径
void AlgorithmHatchingRing::createSpiral(PocketTree &outerTree, const double &powDis)
{
    for(auto &branch : outerTree.branchList)
    {
        branch.spiralPath.clear();
        branch.calcNodeWndPos_Spiral(powDis);
        branch.createSpiral(powDis);
    }
}

/// 创建费马螺旋路径
void AlgorithmHatchingRing::createFermatSpiral(PocketTree &outerTree, const double &powDis)
{
    for(auto &branch : outerTree.branchList)
    {
        branch.calcNodeWndPos(powDis);
        branch.createFermatSpiral(powDis);
    }
}

/// 合并内外口袋树结构
/// @param outerTree 输出参数,外部口袋树
/// @param innerTree 输入参数,内部口袋树
/// @details 实现步骤:
/// 1. 对外部树按面积从大到小排序
/// 2. 第一轮合并尝试
/// 3. 第二轮升级合并尝试
void AlgorithmHatchingRing::mergePocketTree(PocketTree &outerTree, InnerPocketTree &innerTree)
{
    // 按面积从大到小排序外部分支
    std::sort(outerTree.branchList.begin(), outerTree.branchList.end(), compareBigger);

    // 第一轮合并
    for(auto &innerBranch : innerTree.branchList)
    {
        for(auto &branch : outerTree.branchList)
        {
            // 尝试添加内部分支
            if(branch.addInnerBranch(innerBranch))
            {
                innerBranch.bMerged = true;
                break;
            }
        }
    }

    // 第二轮升级合并
    for(auto &innerBranch : innerTree.branchList)
    {
        // 跳过已合并的分支
        if(innerBranch.bMerged) continue;
        
        bool bMerged = false;
        for(auto &branch : outerTree.branchList)
        {
            // 尝试升级方式添加内部分支
            if(branch.addInnerBranch_upgrade(innerBranch))
            {
                bMerged = true;
                break;
            }
        }
        
        // 输出未能合并的警告
        if(!bMerged) 
            qDebug() << "!!!!!!!!!!!!!!!!Can not merge!!!!!!!!!!!!!!!!";
    }
}


#include <QTime>
void drawPathOffset(const Path &path, const IntPoint &offsetCoor, const ulong &ms)
{
#ifdef USE_PROCESSOR_EXTEND
    uint nPtCnt = path.size();
    if(nPtCnt < 2) return;

    qint64 nLastX = path[0].X, nLastY = path[0].Y;
    uint iPt = 1;
    srand(uint(QTime::currentTime().msec()));

//    quint8 base = rand() % 200;
//    quint8 r = base + quint8(rand() % (256 - base));

//    quint8 g = base + quint8(rand() % (256 - base));

//    base = 255 - base;
//    quint8 b = base + quint8(rand() % (256 - base));

    quint8 r = 0, g = 0, b = 250;

    do
    {
        glProcessor->drawLines(nLastX + offsetCoor.X, nLastY + offsetCoor.Y,
                               path[iPt].X + offsetCoor.X, path[iPt].Y + offsetCoor.Y, r, g, b);
        nLastX = path[iPt].X;
        nLastY = path[iPt].Y;
        QThread::msleep(ms);
    }
    while(++ iPt < nPtCnt);
#endif
}
