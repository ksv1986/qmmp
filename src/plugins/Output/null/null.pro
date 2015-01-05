include(../../plugins.pri)

HEADERS += outputnullfactory.h \
           outputnull.h

SOURCES += outputnullfactory.cpp \
           outputnull.cpp


TARGET=$$PLUGINS_PREFIX/Output/null

INCLUDEPATH += ../../../
QMAKE_LIBDIR += ../../../../lib

CONFIG += warn_on \
thread \
plugin

TEMPLATE = lib

RESOURCES = translations/translations.qrc

unix {
    isEmpty (LIB_DIR){
    LIB_DIR = /lib
    }

    target.path = $$LIB_DIR/qmmp/Output
    INSTALLS += target
    LIBS += -lqmmp
    QMAKE_CLEAN =$$PLUGINS_PREFIX/Output/libnull.so
}

win32 {
    LIBS += -lqmmp0
    QMAKE_LIBDIR += ../../../../bin
}
