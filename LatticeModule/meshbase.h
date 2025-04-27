#ifndef MESHBASE_H
#define MESHBASE_H

#include <QSharedPointer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector>
#include <QPair>
#include <QMap>

#include <QDebug>

#include "clipper.hpp"

namespace meshdef {

struct Vertex;
struct Edge;
struct Face;
struct BoundingBox;
struct CalcFaceResult;
struct ClipPathResult;

typedef QSharedPointer<Vertex> VertexPtr;
typedef QSharedPointer<Edge> EdgePtr;
typedef QSharedPointer<Face> FacePtr;

struct BoundingBox {
    float _minX = FLT_MAX;
    float _minY = FLT_MAX;
    float _minZ = FLT_MAX;
    float _maxX = FLT_MIN;
    float _maxY = FLT_MIN;
    float _maxZ = FLT_MIN;
};

enum EdgeResult {
    ERT_NOINSECITON = 0,
    ERT_ONEDGE,
    ERT_ONVERTEX,
};

enum FaceResult {
    FRT_FOUNDNONE = 0,
    FRT_FOUNDFIRST,
    FRT_FOUNDALL
};

struct Vertex {
    QVector3D _coor = QVector3D();
    QList<Face *> _faceList;

    Vertex() = default;
    Vertex(const float &x, const float &y, const float &z) :
        _coor(x, y, z) {}

    void transform(const QMatrix4x4 &mat);
};

struct CalcEdgeResult {
    Edge *_edge = nullptr;
    EdgeResult _result = ERT_NOINSECITON;
    Vertex *_vertex = nullptr;
    QVector3D _coor;

    void setVertex(Vertex *v) {
        _result = ERT_ONVERTEX;
        _vertex = v;
        _coor = v->_coor;
    }
};

struct Edge {
    QVector<Vertex *> _vertexs;
    Face *_curFace = nullptr;
    Face *_nextFace = nullptr;

    Edge() = default;
    Edge(Vertex *v1, Vertex *v2, Face *curFace) {
        _vertexs.resize(2);
        _vertexs[0] = v1;
        _vertexs[1] = v2;
        _curFace = curFace;
    }
    bool calcIntersection(CalcEdgeResult &, ClipPathResult &);
};

struct Face {
    QVector3D _normal;
    QVector<Vertex *> _vertexs;
    QVector<Edge *> _edges;

    Face() = default;
    Face(Vertex *v1, Vertex *v2, Vertex *v3);


    bool isFaceOnLayerHei(const float &);
    void updateNormal();
    bool calcIntersection(ClipPathResult &);
    bool updateClipResult(ClipPathResult &, const QVector<CalcEdgeResult> &);
    bool updateClipResult(ClipPathResult &, const CalcEdgeResult &, const CalcEdgeResult &);
};

struct CalcFaceResult {
    FaceResult _result = FRT_FOUNDNONE;
    QVector3D _startPt;
    QVector3D _endPt;

    void initResult() { _result = FRT_FOUNDNONE; }
    void setFirstPt(const QVector3D &pt)  {
        _startPt = std::move(pt);
        _result = FRT_FOUNDFIRST;
    }
};

struct ClipPathResult {
    float _layerHei = 0;
    QSet<long long> _usedFaceSet;
    QSet<long long> _usedVertexSet;
    QSet<QPair<long long, long long>> _usedEdgeSet;
    CalcEdgeResult _startCoor;

    bool _started = false;
    QVector<CalcEdgeResult> _edgeResultVec;
    ClipperLib::DoublePath _path;
};

struct Part {
    QList<VertexPtr> _vertexList;
    QMap<QPair<long long, long long>, EdgePtr> _edgeMap;
    QList<FacePtr> _faceList;

    void createPartInfo(const QVector<QVector3D> &vertexs,
                        const QVector<QVector<int>> &faces);
    void tranformPart(const QMatrix4x4 &mat);
    ClipperLib::DoublePath cutPartOnZ(const float &layerHei);

private:
    void findNextIntersection(Face *, ClipPathResult &);
    void updateFaceInfo(Vertex *, Vertex *, Vertex *);
};
}

#endif // MESHBASE_H
