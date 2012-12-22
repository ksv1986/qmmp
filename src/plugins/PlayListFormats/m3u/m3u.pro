include(../../plugins.pri)

CONFIG += release \
warn_on \
plugin  \
 lib

TARGET =$$PLUGINS_PREFIX/PlayListFormats/m3uplaylistformat

TEMPLATE = lib
QMAKE_LIBDIR += ../../../../lib

INCLUDEPATH += ../../../../src

HEADERS += m3uplaylistformat.h

SOURCES += m3uplaylistformat.cpp

unix {
    isEmpty(LIB_DIR){
        LIB_DIR = /lib
    }
    target.path = $$LIB_DIR/qmmp/PlaylistFormats
    INSTALLS += target

    QMAKE_CLEAN =$$PLUGINS_PREFIX/General/libm3uplaylistformat.so
    LIBS += -lqmmpui
}

win32 {
    LIBS += -lqmmpui0
    QMAKE_LIBDIR += ../../../../bin
}
