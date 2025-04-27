#include "splicingabstract.h"
#include "bpccommon.h"

void RandomLineSplicing::initSplicingRanges(const int &lineType, const int64_t &minPos, const int64_t &maxPos,
                                            const double &overlap, const double &allowedRange, const double &minOffset)
{
    double tempRange = allowedRange;
    if(tempRange < 0) tempRange = 0;
    if(tempRange > 100) tempRange = 100;

    splicingBaseSettings.splicingLineType = lineType;
    splicingBaseSettings.minRangePos = minPos;
    splicingBaseSettings.maxRangePos = maxPos;
    splicingBaseSettings.overlap = qRound64(UNITSPRECISION * overlap);
    splicingBaseSettings.overlap_border = qRound64(UNITSPRECISION * overlap);
    splicingBaseSettings.overlap_support = qRound64(UNITSPRECISION * overlap);

    auto centerPos = (minPos + maxPos) >> 1;
    const auto range1 = qRound64((maxPos - minPos) * tempRange / 100);
    const auto range2 = (maxPos - minPos) - qRound64(UNITSPRECISION * overlap) * 2;

    auto range = qMin(range1, range2);
    range = range >> 1;
    if(range  < 0) range  = 0;
    splicingBaseSettings.minSplicingPos = centerPos - range;
    splicingBaseSettings.maxSplicingPos = centerPos + range;
    minSplicingLineOffset = qRound64(minOffset * UNITSPRECISION);
}

void RandomLineSplicing::calcNextPos()
{
    if(splicingBaseSettings.curSplicingPos >= splicingBaseSettings.minSplicingPos &&
        splicingBaseSettings.curSplicingPos <= splicingBaseSettings.maxSplicingPos)
    {
        auto rangeType = 0;
        auto range1_max = splicingBaseSettings.curSplicingPos - minSplicingLineOffset;
        auto range2_min = splicingBaseSettings.curSplicingPos + minSplicingLineOffset;
        if(range1_max > splicingBaseSettings.minSplicingPos) rangeType |= 0x1;
        if(range2_min < splicingBaseSettings.maxSplicingPos) rangeType |= 0x2;
        auto randV = qrand();
        switch(rangeType)
        {
        case 0x1:
        {
            auto tempPos = splicingBaseSettings.minSplicingPos +
                           (randV % (range1_max - splicingBaseSettings.minSplicingPos));
            splicingBaseSettings.curSplicingPos = tempPos;
        }
            break;
        case 0x2:
        {
            auto tempPos = range2_min +
                           (randV % (splicingBaseSettings.maxSplicingPos - range2_min));
            splicingBaseSettings.curSplicingPos = tempPos;
        }
            break;
        case 0x3:
        {
            auto range = (splicingBaseSettings.maxSplicingPos - range2_min) +
                         (range1_max - splicingBaseSettings.minSplicingPos);
            double ratio = double(range1_max - splicingBaseSettings.minSplicingPos) / range;

            if((randV % 100) * 0.01 < ratio)
            {
                auto tempPos = splicingBaseSettings.minSplicingPos +
                               (randV % (range1_max - splicingBaseSettings.minSplicingPos));
                splicingBaseSettings.curSplicingPos = tempPos;
            }
            else
            {
                auto tempPos = range2_min +
                               (randV % (splicingBaseSettings.maxSplicingPos - range2_min));
                splicingBaseSettings.curSplicingPos = tempPos;
            }
        }
            break;
        default:
        {
            auto tempPos = splicingBaseSettings.minSplicingPos +
                           (randV % (splicingBaseSettings.maxSplicingPos - splicingBaseSettings.minSplicingPos));
            splicingBaseSettings.curSplicingPos = tempPos;
        }
            break;
        }
    }
    else
    {
        auto randV = qrand();
        auto tempPos = splicingBaseSettings.minSplicingPos +
                       (randV % (splicingBaseSettings.maxSplicingPos - splicingBaseSettings.minSplicingPos));
        splicingBaseSettings.curSplicingPos = tempPos;
    }
}

//PeriodicLine
void PeriodicLine::initSplicingRanges(const int &lineType, const int64_t &minPos, const int64_t &maxPos,
                                      const double &overlap, const double &allowedRange, const int &repeatCnt,
                                      const double &initialPos, const double &stepLength)
{
    double tempRange = allowedRange;
    if(tempRange < 0) tempRange = 0;
    if(tempRange > 100) tempRange = 100;

    splicingBaseSettings.splicingLineType = lineType;
    splicingBaseSettings.minRangePos = minPos;
    splicingBaseSettings.maxRangePos = maxPos;
    splicingBaseSettings.overlap = qRound64(UNITSPRECISION * overlap);
    splicingBaseSettings.overlap_border = qRound64(UNITSPRECISION * overlap);
    splicingBaseSettings.overlap_support = qRound64(UNITSPRECISION * overlap);

    auto centerPos = (minPos + maxPos) >> 1;
    const auto range1 = qRound64((maxPos - minPos) * tempRange / 100);
    const auto range2 = (maxPos - minPos) - qRound64(UNITSPRECISION * overlap) * 2;

    auto range = qMin(range1, range2);
    range = range >> 1;
    if(range  < 0) range  = 0;
    splicingBaseSettings.minSplicingPos = centerPos - range;
    splicingBaseSettings.maxSplicingPos = centerPos + range;

    _curStepIndex = 0;
    _repeatCnt = (repeatCnt < 1) ? 1 : repeatCnt;
    _stepLength = qRound64(stepLength * UNITSPRECISION);
    _initialPos = splicingBaseSettings.minSplicingPos
                  + qRound64((splicingBaseSettings.maxSplicingPos - splicingBaseSettings.minSplicingPos) * (initialPos / 100.0));
    if(_initialPos < splicingBaseSettings.minSplicingPos) _initialPos = splicingBaseSettings.minSplicingPos;
    if(_initialPos > splicingBaseSettings.maxSplicingPos) _initialPos = splicingBaseSettings.maxSplicingPos;
}

void PeriodicLine::calcNextPos()
{
    auto tempPos = _curStepIndex * _stepLength + _initialPos;
    if(tempPos < _initialPos || tempPos >= splicingBaseSettings.maxSplicingPos)
    {
        tempPos = _initialPos;
        _curStepIndex = 0;
    }
    splicingBaseSettings.curSplicingPos = tempPos;

    ++ _curStepIndex;
    if(_curStepIndex >= _repeatCnt) _curStepIndex = 0;
}
