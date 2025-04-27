#ifndef DIVIDERHEADER_HHH_HH_H
#define DIVIDERHEADER_HHH_HH_H

#include "Clipper/clipper.hpp"

#include <QDebug>

using namespace ClipperLib;

struct MinErrResult {
    double _error = std::numeric_limits<double>::max();
    double _pos = 0;;

    void updateResult(const double &pos, const double &error) {
        auto tempError = fabs(error);
        if (tempError < _error) {
            _pos = pos;
            _error = tempError;
        }
    }
};

struct EaringTriangle {
    long long _minX = std::numeric_limits<long long>::max();
    long long _minY = std::numeric_limits<long long>::max();
    long long _maxX = std::numeric_limits<long long>::min();
    long long _maxY = std::numeric_limits<long long>::min();

    double _area = 0.0;
    Path _path;

    EaringTriangle() = default;
    EaringTriangle(const IntPoint &p1, const IntPoint &p2, const IntPoint &p3,
                   const double &area) {
        initTriangle(p1, p2, p3, area);
    }
    virtual ~EaringTriangle() {}

    void initTriangle(const IntPoint &p1, const IntPoint &p2, const IntPoint &p3,
                      const double &area) {
        _path.push_back(p1);
        _path.push_back(p2);
        _path.push_back(p3);
        _minX = std::min(_minX, p1.X);
        _minX = std::min(_minX, p2.X);
        _minX = std::min(_minX, p3.X);
        _maxX = std::max(_maxX, p1.X);
        _maxX = std::max(_maxX, p2.X);
        _maxX = std::max(_maxX, p3.X);

        _minY = std::min(_minY, p1.Y);
        _minY = std::min(_minY, p2.Y);
        _minY = std::min(_minY, p3.Y);
        _maxY = std::max(_maxY, p1.Y);
        _maxY = std::max(_maxY, p2.Y);
        _maxY = std::max(_maxY, p3.Y);
        _area = area;
    }

    bool ptOutTheTriangle(const IntPoint &pt) {
        if (pt.X < _minX) return true;
        if (pt.X > _maxX) return true;
        if (pt.Y < _minY) return true;
        if (pt.Y > _maxY) return true;

        auto type = PointInPolygon(pt, _path);
        return (0 == type || -1 == type);
    }
};

inline QDebug operator<<(QDebug debug, const EaringTriangle &earingTri)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "{ " << earingTri._minY << ", "
                    << earingTri._maxY << ", "
                    << earingTri._area << "}";
    return debug;
}

inline QDebug operator<<(QDebug debug, const IntPoint &pt)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "( " << pt.X << ", "
                    << pt.Y << ")";
    return debug;
}

inline QDebug operator<<(QDebug debug, const DoublePoint &pt)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "( " << pt.X << ", "
                    << pt.Y << ")";
    return debug;
}

#endif
