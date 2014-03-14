include(../../../qmmp.pri)

SUBDIRS += mad cue vorbis sndfile wavpack
TEMPLATE = subdirs

contains(CONFIG, FLAC_PLUGIN){
    SUBDIRS += flac
}

contains(CONFIG, MUSEPACK_PLUGIN){
    SUBDIRS += mpc
}

contains(CONFIG, MODPLUG_PLUGIN){
    SUBDIRS += modplug
}

contains(CONFIG, FFMPEG_PLUGIN){
   contains(CONFIG, FFMPEG_LEGACY){
       SUBDIRS += ffmpeg_legacy
    }else{
       SUBDIRS += ffmpeg
    }
}

contains(CONFIG, GME_PLUGIN){
  SUBDIRS += gme
}

contains(CONFIG, OPUS_PLUGIN){
  SUBDIRS += opus
}

contains(CONFIG, CDAUDIO_PLUGIN){
  SUBDIRS += cdaudio
}

contains(CONFIG, SID_PLUGIN){
  SUBDIRS += sid
}

unix{

contains(CONFIG, AAC_PLUGIN){
  SUBDIRS += aac
}


contains(CONFIG, WILDMIDI_PLUGIN){
  SUBDIRS += wildmidi
}



}
