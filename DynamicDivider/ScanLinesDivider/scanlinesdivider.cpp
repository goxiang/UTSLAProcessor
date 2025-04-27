#include "scanlinesdivider.h"

#include <QDebug>

class MemGuard {
public:
    MemGuard(std::function<void()> &&func) {
        _func = std::move(func);
    }
    ~MemGuard() { _func(); }
private:
    std::function<void()> _func = [](){};
};

struct ScanLineInfo {
    long long _pathIndex = 0;
    double _jumpLength = 0.0;
    double _markLength = 0.0;
    double _totalLength = 0.0;
};

struct DividerPriv {
    std::vector<ScanLineInfo> _scanLineVec;

    double createScanLineVec(const ClipperLib::Paths &paths) {
        bool firstAction = true;
        long long lastX = 0;
        long long lastY = 0;

        double totalLength = 0;
        for (const auto &path : paths) {
            ScanLineInfo lineInfo;
            MemGuard memGuard([&lineInfo, this](){ _scanLineVec.push_back(lineInfo); });

            if (path.size() < 2) continue;

            auto it = path.cbegin();
            if (firstAction) {
                lineInfo._jumpLength = 0;
                firstAction = false;
                lastX = it->X;
                lastY = it->Y;
                continue;
            }
            else {
                lineInfo._jumpLength = sqrt(pow(it->X - lastX, 2) + pow(it->Y - lastY, 2));
                totalLength += lineInfo._jumpLength;
                lastX = it->X;
                lastY = it->Y;
            }

            do {
                ++ it;
                if (it == path.cend()) break;
                auto length = sqrt(pow(it->X - lastX, 2) + pow(it->Y - lastY, 2));
                lineInfo._markLength += length;
                totalLength += length;
                lastX = it->X;
                lastY = it->Y;
            }
            while(true);

            lineInfo._totalLength = totalLength;
        }
        return totalLength;
    }

    void calcPosVec(std::vector<PartResult> &disInfoVec,
                    std::vector<long long> &posVec,
                    const double &totalLength) {
        long long lastPos = 0;
        long long pos = 0;
        auto totalSz = _scanLineVec.size();

        double lastWeight = 0;

        int disNum = disInfoVec.size() - 1;

        for (int i = 0; i < disNum; ++ i) {
            lastWeight += disInfoVec[i]._factor;
            auto targetLength = totalLength * lastWeight;

            while (true) {
                if (_scanLineVec[pos]._totalLength > targetLength) {
                    posVec.push_back(lastPos);
                    break;
                }
                lastPos = pos;
                ++ pos;
                if (pos >= totalSz) break;
            }
        }
        posVec.push_back(totalSz - 1);

        if (disInfoVec.size() != (posVec.size())) qDebug() << "[Error]" << "posVec" << posVec << disInfoVec << disInfoVec.size() << totalSz;
    }
};

void ScanLinesDivider::calcScanLinesPos(const ClipperLib::Paths &paths,
                                       std::vector<PartResult> &disInfoVec,
                                       std::vector<long long> &posVec)
{
    std::sort(disInfoVec.begin(), disInfoVec.end(),
              [](const PartResult &res1, const PartResult &res2) {
        return res1._factor  > res2._factor;
    });

    DividerPriv priv;
    double totalLength = priv.createScanLineVec(paths);
    priv.calcPosVec(disInfoVec, posVec, totalLength);
}
