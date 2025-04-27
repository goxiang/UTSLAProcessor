#ifndef SCANLINESDIVIDER_H
#define SCANLINESDIVIDER_H

#include "../algorithmdistribution.h"
#include "Clipper/clipper.hpp"

using namespace ClipperLib;

class ScanLinesDivider
{
public:
    ScanLinesDivider() = default;

    static void calcScanLinesPos(const ClipperLib::Paths &path, std::vector<PartResult> &, std::vector<long long> &);
};

#endif // SCANLINESDIVIDER_H
