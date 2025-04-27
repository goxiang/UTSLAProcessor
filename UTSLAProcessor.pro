QT += concurrent gui

TEMPLATE = lib
DEFINES += UTSLAPROCESSOR_LIBRARY

CONFIG += c++11

CONFIG(release, debug|release): DESTDIR = $$PWD/../../../../OUTPUT/release
else:CONFIG(debug, debug|release): DESTDIR = $$PWD/../../../../OUTPUT/debug
QMAKE_LFLAGS += -Wl,--no-undefined
DEFINES += UTECK_UBUNTU_PROJECT
DEFINES += USE_CLIPPER_V2

VERSION = 1.4.0.18
QMAKE_TARGET_PRODUCT = "UTECKBPC"
QMAKE_TARGET_COMPANY = "UnionTech"
QMAKE_TARGET_DESCRIPTION = "DLL"
QMAKE_TARGET_COPYRIGHT = "UnionTech"
TARGET_EXT = .dll

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIsNIN only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../Common/Clipper/clipper.cpp \
    DynamicDivider/PolygonsDivider/polygonsdivider.cpp \
    DynamicDivider/ScanLinesDivider/scanlinesdivider.cpp \
    DynamicDivider/TargetedDistribution/targeteddistribution.cpp \
    DynamicDivider/WaterDistribution/waterdistributionpriv.cpp \
    DynamicDivider/algorithmdistribution.cpp \
    DynamicDivider/dividerprocessor.cpp \
    LatticeModule/latticeinterface.cpp \
    LatticeModule/latticsdiamond.cpp \
    LatticeModule/meshbase.cpp \
    ScanLinesSortor/quadtreedef.cpp \
    ScanLinesSortor/quadtreesortor.cpp \
    ScanLinesSortor/scanlinessortor.cpp \
    SelfAdaptiveModule/selfadaptivemodule.cpp \
    SplicingModule/splicingabstract.cpp \
    SplicingModule/splicingmodulepriv.cpp \
    algorithmapplication.cpp \
    algorithmbase.cpp \
    algorithmchecker.cpp \
    algorithmhatching.cpp \
    algorithmhatchingring.cpp \
    algorithmstrip.cpp \
    clipper2/clipper.engine.cpp \
    clipper2/clipper.offset.cpp \
    clipper2/clipper.rectclip.cpp \
    meshinfo.cpp \
    polygonstartchanger.cpp \
    publicheader.cpp \
    slaprocessorextend.cpp \
    sljobfilewriter.cpp \
    SplicingModule\slmsplicingmodule.cpp \
    uspfilewriter.cpp \
    utslaprocessor.cpp \
    utslaprocessorprivate.cpp \
    writejfile.cpp \
    writeuff.cpp

HEADERS += \
    ../../Common/Clipper/clipper.hpp \
    DynamicDivider/PolygonsDivider/dividerheader.h \
    DynamicDivider/PolygonsDivider/polygonsdivider.h \
    DynamicDivider/ScanLinesDivider/scanlinesdivider.h \
    DynamicDivider/TargetedDistribution/targeteddistribution.h \
    DynamicDivider/WaterDistribution/waterdistributionpriv.h \
    DynamicDivider/algorithmdistribution.h \
    DynamicDivider/dividerprocessor.h \
    LatticeModule/latticeinterface.h \
    LatticeModule/latticsabstract.h \
    LatticeModule/latticsdiamond.h \
    LatticeModule/meshbase.h \
    ScanLinesSortor/quadtreedef.h \
    ScanLinesSortor/quadtreesortor.h \
    ScanLinesSortor/scanlinessortor.h \
    SelfAdaptiveModule/selfadaptivemodule.h \
    SplicingModule/splicingabstract.h \
    SplicingModule/splicingheader.h \
    SplicingModule/splicingmodulepriv.h \
    UTSLAProcessor_global.h \
    algorithmapplication.h \
    algorithmbase.h \
    algorithmchecker.h \
    algorithmhatching.h \
    algorithmhatchingring.h \
    algorithmstrip.h \
    clipper2/clipper.core.h \
    clipper2/clipper.engine.h \
    clipper2/clipper.h \
    clipper2/clipper.minkowski.h \
    clipper2/clipper.offset.h \
    clipper2/clipper.rectclip.h \
    clipper2/clipper.version.h \
    meshinfo.h \
    pathparameters.h \
    polygonstartchanger.h \
    publicheader.h \
    slaprocessorextend.h \
    slascaninfodef.h \
    sljobfilewriter.h \
    SplicingModule\slmsplicingmodule.h \
    SplicingModule\splicingfixedmode.h \
    uspfiledef.h \
    uspfilewriter.h \
    utslaprocessor.h \
    utslaprocessorprivate.h \
    writejfile.h \
    writeuff.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $$PWD/../../Common
INCLUDEPATH += $$PWD/../../Common/Clipper
INCLUDEPATH += $$PWD/../../Common/BPCCommon

win32:CONFIG(release, debug|release): LIBS += -L$$DESTDIR/ -lUTBPProcessBase
else:win32:CONFIG(debug, debug|release): LIBS += -L$$DESTDIR/ -lUTBPProcessBase
else:unix: LIBS += -L$$DESTDIR/ -lUTBPProcessBase

INCLUDEPATH += $$PWD/../UTBPProcessBase
DEPENDPATH += $$PWD/../UTBPProcessBase

win32:CONFIG(release, debug|release): LIBS += -L$$DESTDIR/ -lUTBPParaReader
else:win32:CONFIG(debug, debug|release): LIBS += -L$$DESTDIR/ -lUTBPParaReader
else:unix: LIBS += -L$$DESTDIR/ -lUTBPParaReader

INCLUDEPATH += $$PWD/../BPCParameters/UTBPParaReader
DEPENDPATH += $$PWD/../BPCParameters/UTBPParaReader

win32:CONFIG(release, debug|release): LIBS += -L$$DESTDIR/ -lCommonLib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$DESTDIR/ -lCommonLib
else:unix: LIBS += -L$$DESTDIR/ -lCommonLib

INCLUDEPATH += $$PWD/../BPCParameters/CommonLib
DEPENDPATH += $$PWD/../BPCParameters/CommonLib

win32:CONFIG(release, debug|release): LIBS += -L$$DESTDIR/ -lZipLib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$DESTDIR/ -lZipLib
else:unix: LIBS += -L$$DESTDIR/ -lZipLib

INCLUDEPATH += $$PWD/../../../../Libs/BPC/include
INCLUDEPATH += $$PWD/../../../../Libs/ZipLib/include


INCLUDEPATH += $$PWD/../BPProcessorFactory

LIBS += -L$$DESTDIR/ -lSLJFileParser
INCLUDEPATH += $$PWD/../../SLJFileModule/SLJFileParser
INCLUDEPATH += $$PWD/../../SLJFileModule/Common

INCLUDEPATH += $$PWD/SplicingModule

INCLUDEPATH += $$PWD/DynamicDivider
INCLUDEPATH += $$PWD/DynamicDivider/PolygonsDivider
INCLUDEPATH += $$PWD/DynamicDivider/WaterDistribution

QMAKE_CXXFLAGS += -openmp
QMAKE_LFLAGS += -openmp
LIBS += -fopenmp
