#ifndef LATTICSDIAMOND_H
#define LATTICSDIAMOND_H

#include "latticsabstract.h"

///
/// ! @coreclass{LatticsDiamond}
/// 处理特殊晶格结构的专门组件,继承自LatticsAbstract
///
class LatticsDiamond : public LatticsAbstract
{
public:
    LatticsDiamond();
    ~LatticsDiamond() override;

    void initLatticeParas(const QJsonParsing *) override;
    DoublePaths createLattics(const int &) override;
    void identifyLattices(Paths &) override;
    inline const float getSpacing() const override;

private:
    void moveDoublePath(DoublePaths &);

private:
    struct Priv;
    QSharedPointer<Priv> d = nullptr;
};

#endif // LATTICSDIAMOND_H
