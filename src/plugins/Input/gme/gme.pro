include(../../plugins.pri)

HEADERS += decodergmefactory.h \
    decoder_gme.h \
    gmehelper.h \
    settingsdialog.h
SOURCES += decoder_gme.cpp \
    decodergmefactory.cpp \
    gmehelper.cpp \
    settingsdialog.cpp
TARGET = $$PLUGINS_PREFIX/Input/gme
QMAKE_CLEAN = $$PLUGINS_PREFIX/Input/libgme.so
CONFIG += warn_on \
    plugin
TEMPLATE = lib

RESOURCES = translations/translations.qrc

unix{
    target.path = $$LIB_DIR/qmmp/Input
    INSTALLS += target
    LIBS += -lqmmp \
      -L/usr/lib \
      -L/usr/local/lib \
      -I/usr/include \
      -I/usr/local/include \
      -lgme
}

win32 {
    HEADERS += ../../../../src/qmmp/metadatamodel.h \
               ../../../../src/qmmp/decoderfactory.h
    LIBS += -lqmmp0 -lgme.dll
}

FORMS += \
    settingsdialog.ui
