#ifndef DIVIDERPROCESSER_H
#define DIVIDERPROCESSER_H

#include <QSharedPointer>
#include <QSemaphore>
#include <QMap>

#include "utslaprocessorprivate.h"

class UTSLAProcessorPrivate;
struct BuildPart;

class DividerProcessor
{
public:
    DividerProcessor(UTSLAProcessorPrivate *);

    void startProcessing();

private:
    void createPartWriter();
    void processingAllParts();
    void writeFileEnd();
    void updatePartLayerInfo(const int &, const int &, QSharedPointer<BuildPart> &);

    void calcBorderWeight(const QList<int> &, std::vector<std::shared_ptr<DistPartInfo>> &);
    double calcBorderWeight(const QList<int> &);
    double calcSupportWeight(const Paths &);
    double calcUpfaceWeight(const Paths &, const int &);
    double calcDownfaceWeight(const Paths &, const int &);

    void runTaskInConcurrent(const QSet<int> &, const QList<int> &, const QList<int> &, const QMap<int, Paths> &);
    void taskRunner(const int &, const QList<int> &, const QList<int> &, const QMap<int, Paths> &);

private:
    UTSLAProcessorPrivate *_slaPriv = nullptr;

    int _partCnt = 0;
    int _minLayer = -1;
    int _maxLayer = -1;
    QMap<int, QSharedPointer<BuildPart>> _buildPartMap;
    QSemaphore _runningSemaphore;

    QMap<int, QSharedPointer<DivideSolidData>> _divideDataMap;
    std::set<int> _allowedSet_support;
    std::set<int> _allowedSet_hatching;
    std::set<int> _allowedSet_upface;
    std::set<int> _allowedSet_downface;
};

#endif // DIVIDERPROCESSER_H
