#ifndef PUBLICHEADER_HHH_HH_H
#define PUBLICHEADER_HHH_HH_H

#include "algorithmdistribution.h"

#include <QDebug>
#include <set>

// DistributionResult Operate

inline void updateResInfo(DistributionResult &disRes, const double &refWeigth)
{
    disRes._totalWeight = 0.0;
    for (const auto &partInfo : disRes._partsList) {
        disRes._totalWeight += partInfo._realWeight;
    }
}

inline DistributionResult operator<<(DistributionResult &disRes, const PartInfo &partInfo)
{
    disRes._partsList.push_back(partInfo);
    updateResInfo(disRes, 0);
    return disRes;
}

//QDebug
inline QDebug operator<<(QDebug debug, const PartInfo &partInfo)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '{' << partInfo._index << ", "
                    << partInfo._realWeight << ", "
                    << partInfo._weight << ", "
                    << partInfo._factor << '}';

    return debug;
}

inline QDebug operator<<(QDebug debug, const DistributionResult &disRes)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '{'
                    << disRes._totalWeight;

    for (const auto &part : disRes._partsList) {
        debug.nospace() << part;
    }
    debug.nospace() << '}';

    return debug;
}

inline QDebug operator<<(QDebug debug, const SegementsInfo &segementsInfo)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "{ " << segementsInfo._index << ", "
                    << segementsInfo._rateVec << " }";
    return debug;
}

#endif
