#ifndef USPFILEDEF_H
#define USPFILEDEF_H

#include <QtGlobal>
#include <QVector>
#include <QMutex>
#include <QFile>
#include <QSharedPointer>

enum {
    UFFWRITE_BEGIN = 0,
    UFFWRITE_END
};

typedef enum __FILE_SECTIONID {
    SECTION_LAYER       = 0xB,
    SECTION_PARTINDEX,
    SECTION_MINX,
    SECTION_MAXX,
    SECTION_MINY,
    SECTION_MAXY,
    SECTION_LASERPOWER,
    SECTION_MARKSPEED,
    SECTION_FOCUSSHIFT,
    SECTION_CURRENTBEAMINDEX,
    SECTION_LAYERAREAS,
    SECTION_CURRENTSCANNERINDEX,

    SECTION_POLYGON     = 0x30,
    SECTION_POLYGONCOOR,
    SECTION_HATCH,
    SECTION_HATCHCOOR,
    SECTION_SUPPORT,
    SECTION_SUPPORTCOOR,
    SECTION_HATCH_DOWNFILL,
    SECTION_HATCHDOWNFILLCOOR,
    SECTION_HATCH_UPFILL,
    SECTION_HATCHUPFILLCOOR,
    SECTION_HATCH_SMALL,
    SECTION_HATCHSMALL_COOR,
    SECTION_LAYEREND,
    SECTION_FILEEND,

    SECTION_SSHATCH = 0x50, //SolidSupport
    SECTION_SSHATCHCOOR,
    SECTION_SSPOLYGON,
    SECTION_SSPOLYGONCOOR,

    SECTION_SCANTYPE_JUMP   = 0x60,
    SECTION_SCANTYPE_MARK,
    SECTION_SCANTYPE_MARKLOOP,
    SECTION_SCANTYPE_FINISHED
}FILE_SECTIONID;

struct SCANLINE {
    qint8 nLineType;
    int nX;
    int nY;
};

//typedef QSharedPointer<SCANLINE> SCANLINEPTR;

struct UFFWRITEDATA {
    qint8 nMode_Section;
    qint8 nMode_Coor;
    int nPartIndex = 0;
    int nLaserPower;
    int nMarkSpeed;
    QVector<SCANLINE> listSLines;
    UFFWRITEDATA() {
        nMode_Section = -1;
    }
};

typedef QSharedPointer<UFFWRITEDATA> UFFWDATAPTR;
//typedef QVector<UFFWDATAPTR> LISTUFFWDATAPTR;

struct UFILEDATA {
    QMutex gLocker_UFD;
    QVector<UFFWRITEDATA> gListUFileData;
    UFILEDATA() {/*gListUFileData.clear();*/}
};

#include <QDebug>
inline QDebug operator<<(QDebug debug, const SCANLINE &line)
{
    debug = debug.nospace();
    debug << "(" << line.nLineType << "," << line.nX << "," << line.nY << ")";
    return debug;
}

#endif // USPFILEDEF_H
