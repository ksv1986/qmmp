

HEADERS += outputjackfactory.h \
           outputjack.h \
           bio2jack.h

SOURCES += outputjackfactory.cpp \
           outputjack.cpp \
           bio2jack.c

INCLUDEPATH += ../../../
QMAKE_LIBDIR += ../../../
QMAKE_CLEAN = ../libjack.so
CONFIG += release \
warn_on \
thread \
plugin \
link_pkgconfig
DESTDIR = ../
TEMPLATE = lib
LIBS += -lqmmp
PKGCONFIG += jack samplerate
#TRANSLATIONS = translations/jack_plugin_ru.ts
#RESOURCES = translations/translations.qrc

isEmpty (LIB_DIR){
LIB_DIR = /lib
}

target.path = $$LIB_DIR/qmmp/Output
INSTALLS += target
