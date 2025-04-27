#ifndef QUADTREEDEF_H
#define QUADTREEDEF_H

#include <QSet>
#include <QMap>
#include <QVector>
#include <QtGlobal>
#include <QSharedPointer>

#include "bpccommon.h"

#define POPO 0.01
#define MaxDepths 16

struct Node;

enum NodeType {
    LEFT    = 0x0001,
    RIGHT   = 0x0010,
    UP      = 0x0100,
    DOWN    = 0x1000,

    UL      = UP | LEFT,    //0x0101
    UR      = UP | RIGHT,   //0x0110
    DL      = DOWN | LEFT,  //0x1001
    DR      = DOWN | RIGHT, //0x1010
};

struct MPtInfo {
    int _x = 0;
    int _y = 0;

    int _coor_x = 0;
    int _coor_y = 0;

    int _index = 0;
    int _startType = 0;
    QWeakPointer<MPtInfo> _bro;
    Node *_endNode = nullptr;

    MPtInfo() = default;
    MPtInfo(const int &x, const int &y, const int &index, const int &startType);
    void setNode(Node *node);
    MPtInfo &operator=(const MPtInfo &pt);
};
typedef QSharedPointer<MPtInfo> MPtInfoPtr;

struct Node {
    int _depths = 0;
    int _nodeCount = 0;
    int _nodeType = UL;
    Node *_parent;
    QMap<int, QSharedPointer<Node>> _nodeMap;

    int _minX = 0;
    int _maxX = 0;
    int _minY = 0;
    int _maxY = 0;

    Node() = default;
    Node(Node *, const int &, const int &);
    virtual ~Node() = default;

    virtual bool insertNode(const MPtInfoPtr &pt, const int &x0, const int &y0,
                            const int &width, const int &height);
    virtual bool removePt(const MPtInfoPtr &pt);
};

struct EndNode : public Node {
    QSet<MPtInfoPtr> _ptInfoSets;

    explicit EndNode(Node *parent, const int &depth, const int &nodeType) :
        Node(parent, depth, nodeType) { }
    virtual ~EndNode() = default;

    bool insertNode(const MPtInfoPtr &pt, const int &, const int &,
                    const int &, const int &) override;
    bool removePt(const MPtInfoPtr &) override;
};

struct SearchResult {
    int _left_max_x = std::numeric_limits<int>::max();
    int _right_max_x = std::numeric_limits<int>::max();
    int _left_max_y = std::numeric_limits<int>::max();
    int _right_max_y = std::numeric_limits<int>::max();

    double _minDis = std::numeric_limits<double>::max();
    MPtInfoPtr _referPt = nullptr;
    MPtInfoPtr _startPt = nullptr;

    SearchResult() = default;
    SearchResult(const MPtInfoPtr &);
    void calcNearestPt(Node *node);
};

struct NodeSearcher {
    Node *_node = nullptr;
    int _curSearchType = 0;
    QList<int> _nodeTypeList;

    NodeSearcher() = default;
    NodeSearcher(Node *, const int &);

    int getNodeType(const int &nodeType, int &searchType);
    bool findNearNode(SearchResult &searchedInfo);
    bool findNearNode_root(Node *nood, SearchResult &searchedInfo);
    bool findNearNode_root(Node *nood, const int &, SearchResult &searchedInfo);
};

struct MQuadTree {
    int _depths = MaxDepths;
    QSharedPointer<Node> _root;

    MQuadTree(const int &depth = MaxDepths) {
        _depths = depth;
        if (_depths < 4) _depths = 4;
        _root = QSharedPointer<Node>(new Node(nullptr, 0, UL));
    }
    int getNodeCount() { return _root->_nodeCount; }

    bool insertNode(const MPtInfoPtr &);
    bool sortPaths(const MPtInfoPtr &, const Paths &, Paths &);

    template<class T>
    bool sortPaths(const MPtInfoPtr &, const QVector<T> &, QVector<T> &);

private:
    void seekingNeighhorhood(Node *node, SearchResult &);
    void seekingNeighhorhood(Node *node, const int &, SearchResult &);

    bool removePt(const MPtInfoPtr &);
    bool updatePathInfo(const MPtInfoPtr &, const Paths &, Paths &);

    template<class T>
    bool updatePathInfo(const MPtInfoPtr &, const QVector<T> &, QVector<T> &);

private:
    MPtInfoPtr _curPtPtr = nullptr;
    MPtInfoPtr _curBroPtr = nullptr;
};

template<class T>
bool MQuadTree::sortPaths(const MPtInfoPtr &curPt, const QVector<T> &srcVec,
                          QVector<T> &desVec)
{
    updatePathInfo<T>(curPt, srcVec, desVec);

    if (_root->_nodeCount < 1) return false;

    while (_curBroPtr)
    {
        if (getNodeCount() < 1) break;

        SearchResult searchedInfo(_curBroPtr);
        seekingNeighhorhood(_curBroPtr->_endNode, searchedInfo);

        if (false == updatePathInfo(searchedInfo._startPt, srcVec, desVec))
        {
            qDebug() << "!!!--- Error Detected ---!!!" << getNodeCount() << searchedInfo._minDis;
            break;
        }
    }
    return false;
}

template<class T>
bool MQuadTree::updatePathInfo(const MPtInfoPtr &ptPtr, const QVector<T> &srcVec,
                               QVector<T> &desVec)
{
    _curPtPtr = ptPtr;
    if (nullptr == _curPtPtr) return false;
    _curBroPtr = _curPtPtr->_bro.toStrongRef();
    if (nullptr == _curBroPtr) return false;

    removePt(_curPtPtr);

    if (ptPtr->_index >= srcVec.size()) return false;

    if (ptPtr->_startType)
    {
        for (int index = _curBroPtr->_index; index <= _curPtPtr->_index; ++ index)
        {
            auto path = srcVec.at(index);
            // ReversePath(path);
            desVec.push_back(path);
        }
    }
    else
    {
        for (int index = _curPtPtr->_index; index <= _curBroPtr->_index; ++ index)
        {
            desVec.push_back(srcVec.at(index));
        }
    }
    return true;
}

#endif // QUADTREEDEF_H
