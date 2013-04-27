include(../../plugins.pri)

CONFIG += warn_on \
plugin  \
 lib

TARGET =$$PLUGINS_PREFIX/CommandLineOptions/incdecvolumeoption
QMAKE_CLEAN =$$PLUGINS_PREFIX/CommandLineOptions/libincdecvolumeoption.so

TEMPLATE = lib
QMAKE_LIBDIR += ../../../../lib

TRANSLATIONS = translations/incdecvolume_plugin_cs.ts \
               translations/incdecvolume_plugin_de.ts \
               translations/incdecvolume_plugin_pl.ts \
               translations/incdecvolume_plugin_ru.ts \
               translations/incdecvolume_plugin_uk_UA.ts \
               translations/incdecvolume_plugin_zh_TW.ts \
               translations/incdecvolume_plugin_zh_CN.ts \
               translations/incdecvolume_plugin_it.ts \
               translations/incdecvolume_plugin_tr.ts \
               translations/incdecvolume_plugin_lt.ts \
               translations/incdecvolume_plugin_nl.ts \
               translations/incdecvolume_plugin_ja.ts \
               translations/incdecvolume_plugin_es.ts

RESOURCES = translations/translations.qrc

isEmpty(LIB_DIR){
    LIB_DIR = /lib
}
target.path = $$LIB_DIR/qmmp/CommandLineOptions
INSTALLS += target

INCLUDEPATH += ../../../../src

LIBS += -lqmmpui -lqmmp

HEADERS += incdecvolumeoption.h

SOURCES += incdecvolumeoption.cpp
