#ifndef POLYGONSDIVIDER_H
#define POLYGONSDIVIDER_H

#include <memory>
#include <list>

#include "Clipper/clipper.hpp"

struct DivideSolidData {
    ClipperLib::Paths _extendPaths;
    ClipperLib::Paths _paths_Inner;
    ClipperLib::Paths _paths_down;
    ClipperLib::Paths _paths_up;
    int _nIndex_down = -1;
    int _nIndex_up = -1;

    void resetValues() {
        _extendPaths.clear();
        _paths_Inner.clear();
        _paths_down.clear();
        _paths_up.clear();
        _nIndex_down = -1;
        _nIndex_up = -1;
    }
};

struct MinErrResult;
struct EaringTriangle;
class PolygonsDivider
{
public:
    PolygonsDivider();

public:
    long long calcWeightPos(const ClipperLib::Paths &, const double &, int &splitHorizon);
    void setMaxError(const double &);

private:
    void createEaring(const ClipperLib::Paths &);
    long long getWeightPos(const double &, int &);
    void pathsToList(const ClipperLib::Path &, std::list<ClipperLib::IntPoint> &ptList);
    void createEaring_outter(std::list<ClipperLib::IntPoint> &ptList);
    void createEaring_inner(std::list<ClipperLib::IntPoint> &ptList);
    void appendEaringTriangle(const ClipperLib::IntPoint &,  const ClipperLib::IntPoint &,
                              const ClipperLib::IntPoint &, const double &);
    void appendEaringTriangle(const EaringTriangle &);

    void getWeightPos(const double &, const long long &, const long long &, MinErrResult &);
    void calcInsertPt(const ClipperLib::IntPoint &, const ClipperLib::IntPoint &, const long long &,
                      ClipperLib::DoublePoint &);
    void divideTriangle(const ClipperLib::Path &, const double &, const long long &, double &, double &);

    void getWeightPos_ver(const double &, const long long &, const long long &, MinErrResult &);
    void calcInsertPt_ver(const ClipperLib::IntPoint &, const ClipperLib::IntPoint &, const long long &,
                          ClipperLib::DoublePoint &);
    void divideTriangle_ver(const ClipperLib::Path &, const double &, const long long &, double &, double &);

private:
    struct Priv;
    std::shared_ptr<Priv> _d = nullptr;
};

#endif // POLYGONSDIVIDER_H
