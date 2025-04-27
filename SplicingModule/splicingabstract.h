#ifndef SPLICINGABSTRACT_H
#define SPLICINGABSTRACT_H

#include <QtGlobal>

struct SplicingBaseSettings;
class SplicingAbstract
{
public:
    enum SplicingPositionType {
        RT_Left,
        RT_Top,
        RT_Right,
        RT_Bottom
    };

    enum SplicingLineType {
        SLT_Hor,
        SLT_Ver
    };

public:
    virtual ~SplicingAbstract() {}

public:
    virtual void calcNextPos() = 0;
    virtual const int getSplicngLineType() const = 0;
    virtual const int64_t getSplicingPos() const = 0;
    virtual const int64_t getOverlap() const = 0;
    virtual const SplicingBaseSettings &getSplicingSettings() const = 0;

protected:
    SplicingAbstract() {}
};

struct SplicingBaseSettings {
    int splicingLineType = SplicingAbstract::SLT_Hor;
    int64_t overlap = 0;
    int64_t overlap_border = 0;
    int64_t overlap_support = 0;

    int64_t curSplicingPos = -1E10;
    int64_t minSplicingPos = 0;
    int64_t maxSplicingPos = 0;

    int64_t minRangePos = 0;
    int64_t maxRangePos = 0;
};

class RandomLineSplicing : public SplicingAbstract
{
public:
    RandomLineSplicing() {}

public:
    void initSplicingRanges(const int &, const int64_t &, const int64_t &,
                            const double &, const double &, const double &);
    void calcNextPos();
    const int getSplicngLineType() const { return splicingBaseSettings.splicingLineType; }
    const int64_t getSplicingPos() const { return splicingBaseSettings.curSplicingPos; }
    const int64_t getOverlap() const { return splicingBaseSettings.overlap; }
    const SplicingBaseSettings &getSplicingSettings() const { return splicingBaseSettings; }

private:
    SplicingBaseSettings splicingBaseSettings;
    int64_t minSplicingLineOffset = 10;
};

class PeriodicLine : public SplicingAbstract
{
public:
    PeriodicLine() {}

public:
    void initSplicingRanges(const int &, const int64_t &, const int64_t &, const double &,
                            const double &, const int &, const double &, const double &);

    void calcNextPos();
    const int getSplicngLineType() const { return splicingBaseSettings.splicingLineType; }
    const int64_t getSplicingPos() const { return splicingBaseSettings.curSplicingPos; }
    const int64_t getOverlap() const { return splicingBaseSettings.overlap; }
    const SplicingBaseSettings &getSplicingSettings() const { return splicingBaseSettings; }

private:
    SplicingBaseSettings splicingBaseSettings;

    int _curStepIndex = 0;
    int _repeatCnt = 2;
    int64_t _initialPos = 0;
    int64_t _stepLength = 0;
};

#endif // SPLICINGABSTRACT_H
