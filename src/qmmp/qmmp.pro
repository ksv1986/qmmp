unix:include(../../qmmp.pri)
win32:include(../../qmmp.pri)
HEADERS += \
    buffer.h \
    decoder.h \
    output.h \
    outputfactory.h \
    equ/iir_cfs.h \
    equ/iir_fpu.h \
    equ/iir.h \
    decoderfactory.h \
    soundcore.h \
    visual.h \
    visualfactory.h \
    effect.h \
    effectfactory.h \
    statehandler.h \
    qmmp.h \
    fileinfo.h \
    metadatamodel.h \
    tagmodel.h \
    abstractengine.h \
    qmmpaudioengine_p.h \
    audioparameters.h \
    inputsource.h \
    fileinputsource_p.h \
    emptyinputsource_p.h \
    inputsourcefactory.h \
    enginefactory.h \
    metadatamanager.h \
    replaygain_p.h \
    audioconverter_p.h \
    qmmpsettings.h \
    eqsettings.h \
    qmmpevents_p.h \
    volume.h \
    volumecontrol_p.h \
    outputwriter_p.h \
    recycler_p.h \
    qmmpplugincache_p.h \
    channelmap.h \
    channelconverter_p.h
SOURCES += recycler.cpp \
    decoder.cpp \
    output.cpp \
    equ/iir.c \
    equ/iir_cfs.c \
    equ/iir_fpu.c \
    soundcore.cpp \
    visual.cpp \
    effect.cpp \
    statehandler.cpp \
    qmmp.cpp \
    fileinfo.cpp \
    volumecontrol.cpp \
    metadatamodel.cpp \
    tagmodel.cpp \
    abstractengine.cpp \
    qmmpaudioengine.cpp \
    audioparameters.cpp \
    inputsource.cpp \
    fileinputsource.cpp \
    emptyinputsource.cpp \
    metadatamanager.cpp \
    replaygain.cpp \
    audioconverter.cpp \
    qmmpsettings.cpp \
    eqsettings.cpp \
    qmmpevents.cpp \
    outputwriter.cpp \
    qmmpplugincache.cpp \
    channelmap.cpp \
    channelconverter.cpp
FORMS +=
unix:TARGET = ../../lib/qmmp
win32:TARGET = ../../../bin/qmmp
CONFIG += shared \
    warn_on \
    qt \
    thread
TEMPLATE = lib
VERSION = $$QMMP_VERSION

unix {
  isEmpty(LIB_DIR):LIB_DIR = /lib
  DEFINES += LIB_DIR=\\\"$$LIB_DIR\\\"
  SVN_REVISION = $$system(./svn_revision.sh)
  !isEmpty(SVN_REVISION) {
    DEFINES += SVN_REVISION=\\\"$$SVN_REVISION\\\"
  }
}

win32 {
  isEmpty(QMAKE_SH) {
    SVN_REVISION = $$system(svn_revision.cmd)
  } else {
    SVN_REVISION = $$system(sh svn_revision.sh)
  }
  !isEmpty(SVN_REVISION) {
    DEFINES += SVN_REVISION=\\\"$$SVN_REVISION\\\"
  }
}


unix {
    target.path = $$LIB_DIR
    devel.files += \
        abstractengine.h \
        audioparameters.h \
        buffer.h \
        decoderfactory.h \
        decoder.h \
        effectfactory.h \
        effect.h \
        enginefactory.h \
        eqsettings.h \
        fileinfo.h \
        inputsourcefactory.h \
        inputsource.h \
        metadatamanager.h \
        metadatamodel.h \
        outputfactory.h \
        output.h \
        qmmp.h \
        qmmpsettings.h \
        soundcore.h \
        statehandler.h \
        tagmodel.h \
        visualfactory.h \
        visual.h \
        volume.h

    devel.path = /include/qmmp
    INSTALLS += target \
        devel
    DESTDIR = .
}
INCLUDEPATH += ./


unix {
    CONFIG += create_pc create_prl no_install_prl
    QMAKE_PKGCONFIG_NAME = qmmp
    QMAKE_PKGCONFIG_DESCRIPTION = qmmp core library
    QMAKE_PKGCONFIG_REQUIRES = QtCore QtGui
    QMAKE_PKGCONFIG_DESTDIR = pkgconfig
}
