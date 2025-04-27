#ifndef SELFADAPTIVEMODULE_H
#define SELFADAPTIVEMODULE_H

#include "bppextendparameters.h"

class SelfAdaptiveModule
{
public:
    SelfAdaptiveModule();

    inline SelfAdaptiveFactor &getSelfAdaptive() { return _selfAdaptiveParas; }
    VarioLayerThicknessSpeedFactor getMarkSpeedRatio(const double &);
    void createThicknessList();

private:
    QList<double> listLayersThickness;
    SelfAdaptiveFactor _selfAdaptiveParas;
};

#endif // SELFADAPTIVEMODULE_H
