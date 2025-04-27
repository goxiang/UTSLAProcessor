#ifndef ALGORITHMHATCHINGRING_H
#define ALGORITHMHATCHINGRING_H

#include "algorithmbase.h"

#include <QList>
#include <QDebug>

struct WndInfo {
    uint nWndBegin  = 0;
    uint nWndEnd1   = 0;
    uint nWndEnd2   = 0;
};

inline QDebug operator<<(QDebug debug, const WndInfo &wndInfo)
{
    debug << "WndInfo" << "{ ";
    debug << wndInfo.nWndBegin << " " << wndInfo.nWndEnd1
           << " "<< wndInfo.nWndEnd2;
    debug << " } ";
    return debug;
}

struct RingWndInfo {
    WndInfo curWnd;
    WndInfo bigWnd;
    WndInfo tempWnd;
};

struct WndPos;
struct BiggerWnd;

struct PocketNode {
    int nLevel = 0;         // 层级
    double fArea = 0.0;     // 面积
    BoundingBox bBox;       // 边界框
    Path circle;            // 轮廓路径
    QList<int> subBranchList;  // 子分支列表
    WndInfo wndInfo;        // 窗口信息
    RingWndInfo ringWndInfo;   // 环形窗口信息
    QList<PocketNode *> listNodeWnd;  // 节点窗口列表

    PocketNode() = default;
    PocketNode(const int &level, const double &area,
               const BoundingBox &box, const Path &path);
    IntPoint firstCoor() const;
    bool containsCoor(const IntPoint &coor) const;
};

struct PocketBranch {
    enum {
        Inner = 0,
        Outer
    };
    bool bMerged = false;   // 是否已合并
    qint8 mCircleType = Outer;  // 轮廓类型
    QList<PocketNode> mNodeList;  // 节点列表
    QList<PocketBranch> mSubBranchList;  // 子分支列表
    QList<int> mNestingList;    // 嵌套列表
    QList<int> mTempIndexList;
    Path spiralPath;
    Path fermatSpiralPath;

    PocketBranch() = default;
    PocketBranch(const qint8 &circleType, const PocketNode &node);
    bool containsNode(const IntPoint &coor, const int &index);
    bool containsInNode(PocketNode &node, const int &index) const;
    void updateNestingList(const int &thisIndex, PocketBranch &branch, const int &subIndex);
    void addNode(QList<PocketNode> &tempNodeList);
    bool addInnerBranch(PocketBranch &innerBranch);
    bool addInnerBranch_upgrade(PocketBranch &innerBranch);
    void updateBranchOrder();
    double maxArea() const;
    void getNodeCnt(int &nodeCnt) const;
    void createSpiral(const double &);
    void calcNodeWndPos_Spiral(const double &);
    void calcClosedPt(Path &bigger, WndPos &, Path &smaller, WndPos &wndPos, const uint &minPos, const uint &maxPos, const double &, QList<BiggerWnd> &);
    void calcClosedPtInvert(Path &bigger, WndPos &, Path &smaller, WndPos &wndPos, const uint &minPos, const uint &maxPos, const double &, QList<BiggerWnd> &);
    void calcSpiralJoinPoint(const int &curIndex, int &preIndex, uint &beginPos, uint &endPos, const double &, const bool &maxCircle = false);

    void createFermatSpiral(const double &);
    void calcNodeWndPos(const double &);
    void calcNodeWndPos(PocketNode &, PocketNode &, const double &, const bool &);
    void calcBranchWndPos(PocketNode &, PocketNode &, const double &, const int &);
};

struct PocketTree {
    QList<PocketBranch> branchList;
    void appendBranch(const PocketBranch &branch);
    int getNodeCnt(int &nodeCnt);
    void updateBranchOrder();
};

struct InnerPocketTree {
    QMap<int, PocketBranch> branchList;
    int nIndex = 0;
    void appendBranch(const PocketBranch &branch);
    int getNodeCnt(int &nodeCnt);
    void updateNestingList();
    void removeNestingList(QList<int> &subBranchList);
};

struct SCANLINE;
///
/// ! @coreclass{AlgorithmHatchingRing}
/// 处理口袋(Pocket)结构的填充路径生成,继承自AlgorithmBase
///
class AlgorithmHatchingRing : public AlgorithmBase
{
public:
    AlgorithmHatchingRing() = default;

protected:
    void createPocketTree(const Paths &curPaths, PocketTree &outerTree, InnerPocketTree &innerTree);
    void updatePocketTree(const Paths &curPaths, PocketTree &outerTree, InnerPocketTree &innerTree,
                          const int &level);
    void sortPocketTree(PocketTree &outerTree, InnerPocketTree &innerTree);
    void createSpiral(PocketTree &outerTree, const double &powDis);
    void createFermatSpiral(PocketTree &outerTree, const double &powDis);

private:
    void mergePocketTree(PocketTree &outerTree, InnerPocketTree &innerTree);
};

inline QDebug operator<<(QDebug debug, const PocketBranch &branch)
{
    debug << "branch { ";
    debug << (branch.mCircleType ? "Outer" : "Inner") << "; ";
    debug << "Start: " << branch.mNodeList.first().nLevel << " " << branch.mNodeList.first().fArea << "; ";
    debug << "End: " << branch.mNodeList.last().nLevel << " " << branch.mNodeList.last().fArea  << "; ";
    debug << "CurWnd: " << branch.mNodeList.first().ringWndInfo.curWnd  << "; ";
    debug << "TempWnd: " << branch.mNodeList.first().ringWndInfo.tempWnd  << "; ";

    if(branch.mSubBranchList.size())
    {
        debug << "\nList: " << branch.mSubBranchList.size() << "{\n";
        for(const auto &subBranch : branch.mSubBranchList) {
            debug << subBranch;
        }
        debug << "} ";
    }
    debug << "}\n";
    return debug;
}

inline QDebug operator<<(QDebug debug, const PocketTree &tree)
{
    debug << "Tree" << tree.branchList.size() << "{ \n";
    for(const auto &branch : tree.branchList)
    {
        debug << branch;
    }
    debug << "}\n";
    return debug;
}

#endif // ALGORITHMHATCHINGRING_H
