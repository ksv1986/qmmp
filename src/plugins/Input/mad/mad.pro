include(../../plugins.pri)
FORMS += settingsdialog.ui
HEADERS += decodermadfactory.h \
    decoder_mad.h \
    settingsdialog.h \
    tagextractor.h \
    mpegmetadatamodel.h \
    replaygainreader.h
SOURCES += decoder_mad.cpp \
    decodermadfactory.cpp \
    settingsdialog.cpp \
    tagextractor.cpp \
    mpegmetadatamodel.cpp \
    replaygainreader.cpp
TARGET = $$PLUGINS_PREFIX/Input/mad

INCLUDEPATH += ../../../

CONFIG += release \
    warn_on \
    plugin \
    link_pkgconfig
TEMPLATE = lib


TRANSLATIONS = translations/mad_plugin_ru.ts \
    translations/mad_plugin_uk_UA.ts \
    translations/mad_plugin_zh_CN.ts \
    translations/mad_plugin_zh_TW.ts \
    translations/mad_plugin_cs.ts \
    translations/mad_plugin_pl.ts \
    translations/mad_plugin_de.ts \
    translations/mad_plugin_it.ts \
    translations/mad_plugin_tr.ts \
    translations/mad_plugin_tr.ts \
    translations/mad_plugin_lt.ts \
    translations/mad_plugin_nl.ts \
    translations/mad_plugin_ja.ts \
    translations/mad_plugin_es.ts

RESOURCES = translations/translations.qrc
unix { 
    isEmpty(LIB_DIR):LIB_DIR = /lib
    target.path = $$LIB_DIR/qmmp/Input
    INSTALLS += target

    QMAKE_LIBDIR += ../../../../lib
    LIBS += -lqmmp -lmad
    PKGCONFIG += taglib mad
    QMAKE_CLEAN = $$PLUGINS_PREFIX/Input/libmad.so
}

win32 {
    HEADERS += ../../../../src/qmmp/metadatamodel.h \
               ../../../../src/qmmp/decoderfactory.h
    QMAKE_LIBDIR += ../../../../bin
    LIBS += -lqmmp0 -lmad -ltag.dll
}
