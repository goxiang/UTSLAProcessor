#ifndef PATHPARAMETERS_H
#define PATHPARAMETERS_H

#include <QtGlobal>
#include <math.h>
#include "bpccommon.h"
#include "slascaninfodef.h"

class PATHPARAMETERS {
public:
    PATHPARAMETERS() {
        initRC();
    }
    PATHPARAMETERS(Path &nPath) {
        mPath = nPath;
        initRC();
    }

public:
    void initPath(Path &nPath)
    {
        mPath = nPath;
        initRC();
    }
    void initRC()
    {
        mOutRC.minX = 0x7FFFFFFF;
        mOutRC.maxX = 0;
        mOutRC.minY = 0x7FFFFFFF;
        mOutRC.maxY = 0;
        uint nPtCnt = mPath.size();
        for(uint iPt = 0; iPt < nPtCnt; iPt ++)
        {
            mOutRC.minX = qMin(mOutRC.minX, int(mPath[iPt].X));
            mOutRC.maxX = qMax(mOutRC.maxX, int(mPath[iPt].X));
            mOutRC.minY = qMin(mOutRC.minY, int(mPath[iPt].Y));
            mOutRC.maxY = qMax(mOutRC.maxY, int(mPath[iPt].Y));
        }
        m_nCenterX = (mOutRC.minX + mOutRC.maxX) >> 1;
        m_nCenterY = (mOutRC.minY + mOutRC.maxY) >> 1;
    }
    int containtOutRC(BOUNDINGRECT nOutRC) {
        if((mOutRC.minX  < nOutRC.minX && mOutRC.maxX > nOutRC.maxX) &&
                (mOutRC.minY  < nOutRC.minY && mOutRC.maxY > nOutRC.maxY))
        {
            return 1;
        }
        else if((mOutRC.minX  > nOutRC.minX && mOutRC.maxX < nOutRC.maxX) &&
                (mOutRC.minY  > nOutRC.minY && mOutRC.maxY < nOutRC.maxY))
        {
            return -1;
        }
        return 0;
    }
    int nRCArea(int nWid = 100)
    {
        return nWid * ((mOutRC.maxX - mOutRC.minX) + (mOutRC.maxY - mOutRC.minY));
    }
    int isHolesAndLessRadius(double nRadius, double fArea)
    {
        if(nRadius < 1)
        {
            return 0;
        }
        double fArea_Ref = qRound(3.14159265 * nRadius * nRadius);
        if(fArea > fArea_Ref)
        {
            return 0;
        }

        int nDeltaX = (mOutRC.maxX - mOutRC.minX);
        int nDeltaY = (mOutRC.maxY - mOutRC.minY);
        if(nDeltaY < 1)
        {
            return 0;
        }
        double fK = double(nDeltaX) / nDeltaY;
        if(fK < 0.8 || fK > 1.25)
        {
            return 0;
        }

        uint nPtCnt = mPath.size();
        qint64 nRadius_Min = 0xFFFFFFFFFFF;
        qint64 nRadius_Max = 0;
        qint64 nDis = 0;
        int nPtDeltaX = 0, nPtDeltaY = 0;
        for(uint iPt = 0; iPt < nPtCnt; iPt ++)
        {
            nPtDeltaX = int(mPath.at(iPt).X - m_nCenterX);
            nPtDeltaY = int(mPath.at(iPt).Y - m_nCenterY);
            nDis = qRound64(sqrt(pow(nPtDeltaX, 2) + pow(nPtDeltaY, 2)));
            nRadius_Min = qMin(nRadius_Min, nDis);
            nRadius_Max = qMax(nRadius_Max, nDis);
        }
        if(nRadius_Max > 0)
        {
            return (double(nRadius_Min) / double(nRadius_Max)) > 0.8;
        }
        return 0;
    }
    int isLittleGap(int nWid = 100)
    {
        return (fabs(Area(mPath)) < nRCArea(nWid)) ? 1 : 0;
    }
    int isLittleGap(double nWid, double fArea)
    {
        int nDeltaX = (mOutRC.maxX - mOutRC.minX);
        int nDeltaY = (mOutRC.maxY - mOutRC.minY);
        if(nDeltaX < nWid)
        {
            if(nDeltaY < 2 * nWid)
            {
                return 0;
            }
            return 1;
        }
        if(nDeltaY < nWid)
        {
            if(nDeltaX < 2 * nWid)
            {
                return 0;
            }
            return 1;
        }
        double fK = double(nDeltaX) / nDeltaY;
        if(((nDeltaX * nDeltaY) < 50000) && (fK > 0.5 && fK < 2))
        {
            return 0;
        }
        return (abs(fArea) < nRCArea(qRound(nWid))) ? 1 : 0;
    }
    bool operator == (const PATHPARAMETERS &val) const
    {
        return  abs(m_nCenterX - val.m_nCenterX) < 10 &&
                abs(m_nCenterY - val.m_nCenterY) < 10 &&
                abs(mOutRC.minX - val.mOutRC.minX) < 10 &&
                abs(mOutRC.minX - val.mOutRC.minX) < 10 &&
                abs(mOutRC.maxX - val.mOutRC.maxX) < 10 &&
                abs(mOutRC.minY - val.mOutRC.minY) < 10 &&
                abs(mOutRC.maxY - val.mOutRC.maxY) < 10;
    }

public:
    BOUNDINGRECT mOutRC;
    qint64 m_nCenterX;
    qint64 m_nCenterY;

private:
    Path mPath;
};

struct OULTLINEPARAMETERS {
    PATHPARAMETERS topPath;
    QList<PATHPARAMETERS> listPaths;
};

#endif // PATHPARAMETERS_H
