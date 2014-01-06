include(../../plugins.pri)

HEADERS += outputpulseaudiofactory.h \
           outputpulseaudio.h

SOURCES += outputpulseaudiofactory.cpp \
           outputpulseaudio.cpp


TARGET=$$PLUGINS_PREFIX/Output/pulseaudio
QMAKE_CLEAN =$$PLUGINS_PREFIX/Output/libpulseaudio.so

INCLUDEPATH += ../../../
QMAKE_LIBDIR += ../../../../lib

CONFIG += warn_on \
thread \
plugin \
link_pkgconfig

TEMPLATE = lib
LIBS += -lqmmp

PKGCONFIG += libpulse-simple

TRANSLATIONS = translations/pulseaudio_plugin_cs.ts \
               translations/pulseaudio_plugin_de.ts \
               translations/pulseaudio_plugin_zh_CN.ts \
               translations/pulseaudio_plugin_zh_TW.ts \
               translations/pulseaudio_plugin_ru.ts \
               translations/pulseaudio_plugin_pl.ts \
               translations/pulseaudio_plugin_uk_UA.ts \
               translations/pulseaudio_plugin_it.ts \
               translations/pulseaudio_plugin_tr.ts \
               translations/pulseaudio_plugin_lt.ts \
               translations/pulseaudio_plugin_nl.ts \
               translations/pulseaudio_plugin_ja.ts \
               translations/pulseaudio_plugin_es.ts \
               translations/pulseaudio_plugin_sr_BA.ts \
               translations/pulseaudio_plugin_sr_RS.ts

RESOURCES = translations/translations.qrc

isEmpty (LIB_DIR){
LIB_DIR = /lib
}

target.path = $$LIB_DIR/qmmp/Output
INSTALLS += target
