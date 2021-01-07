cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
project(qmmp)

#freebsd support
include_directories(SYSTEM /usr/local/include)
SET(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} /usr/local/include)

#macports support
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} /opt/local/include)
ENDIF()

IF(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
ENDIF()

IF(POLICY CMP0063)
cmake_policy(SET CMP0063 NEW)
ENDIF()

#Qt5 modules
SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} /home/user/qt514/lib/cmake)

#installation paths
include(GNUInstallDirs)

#extract version from qmmp.h
FILE(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/qmmp/qmmp.h"
     QMMP_VERSION_DATA REGEX "^#define[ \t]+QMMP_VERSION_[A-Z]+[ \t]+[0-9]+.*$")

IF(QMMP_VERSION_DATA)
   FOREACH(item IN ITEMS MAJOR MINOR PATCH STABLE)
      STRING(REGEX REPLACE ".*#define[ \t]+QMMP_VERSION_${item}[ \t]+([0-9]+).*"
       "\\1" QMMP_VERSION_${item} ${QMMP_VERSION_DATA})
   ENDFOREACH(item IN ITEMS MAJOR MINOR PATCH)
   SET(QMMP_VERSION "${QMMP_VERSION_MAJOR}.${QMMP_VERSION_MINOR}.${QMMP_VERSION_PATCH}")
   SET(QMMP_SOVERSION "${QMMP_VERSION_MAJOR}")
   IF(QMMP_VERSION_STABLE)
      MESSAGE(STATUS "Qmmp API version: ${QMMP_VERSION} (stable)")
   ELSE(QMMP_VERSION_STABLE)
      MESSAGE(STATUS "Qmmp API version: ${QMMP_VERSION} (development)")
   ENDIF(QMMP_VERSION_STABLE)
ELSE(QMMP_VERSION_DATA)
   MESSAGE(FATAL_ERROR "invalid header")
ENDIF(QMMP_VERSION_DATA)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

INCLUDE(FindPkgConfig)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt5Widgets 5.4.0 REQUIRED)
find_package(Qt5LinguistTools 5.4.0 REQUIRED)
find_package(Qt5Network 5.4.0 REQUIRED)
find_package(Qt5X11Extras 5.4.0)
find_package(Qt5DBus 5.4.0)
find_package(Qt5Multimedia 5.4.0)
find_package(Qt5Sql 5.4.0)
find_package(Qt5WinExtras 5.4.0)
ADD_DEFINITIONS(-DQMMP_WS_X11 -DQT_DISABLE_DEPRECATED_BEFORE=0x050400 -DQT_DEPRECATED_WARNINGS)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_CXX_STANDARD 11)

get_target_property(QT_LRELEASE_EXECUTABLE Qt5::lrelease IMPORTED_LOCATION)

IF(QT_LRELEASE_EXECUTABLE)
MESSAGE(STATUS "Found lrelease executable: " ${QT_LRELEASE_EXECUTABLE})
ELSE(QT_LRELEASE_EXECUTABLE)
MESSAGE(FATAL_ERROR "Could NOT find lrelease executable")
ENDIF(QT_LRELEASE_EXECUTABLE)

MESSAGE(STATUS "Generating translations ...")
IF(SVN_VERSION)
execute_process(COMMAND find ${CMAKE_CURRENT_SOURCE_DIR} -name *.ts COMMAND xargs ${QT_LRELEASE_EXECUTABLE} -silent -nounfinished)
ELSE(SVN_VERSION)
execute_process(COMMAND find ${CMAKE_CURRENT_SOURCE_DIR} -name *.ts COMMAND xargs ${QT_LRELEASE_EXECUTABLE} -silent)
ENDIF(SVN_VERSION)

get_target_property(QT_LIBRARY_DIR Qt5::Core LOCATION)
get_filename_component(QT_LIBRARY_DIR ${QT_LIBRARY_DIR} PATH)

SET(PLUGIN_DIR "${CMAKE_INSTALL_LIBDIR}/qmmp-${QMMP_VERSION_MAJOR}.${QMMP_VERSION_MINOR}" CACHE STRING "plugin path")
SET(USE_SKINNED TRUE CACHE BOOL "enable/disable standard skinned user interface")
SET(USE_DIR_ASSOC TRUE CACHE BOOL "enable/disable inode/directory mime type association")
#SET(APP_NAME_SUFFIX "-1")

pkg_search_module(X11 x11)

add_subdirectory(src/qmmp)
add_subdirectory(src/qmmpui)
add_subdirectory(src/plugins)
add_subdirectory(src/app)
add_subdirectory(doc)

IF(MAD_FOUND)
  SET(MPEG_DECODERS "MAD")
ENDIF(MAD_FOUND)

IF(MPG123_FOUND)
  SET(MPEG_DECODERS ${MPEG_DECODERS} "mpg123")
ENDIF(MPG123_FOUND)

MACRO(PRINT_SUMMARY str USE FOUND)
IF(${USE} AND ${FOUND})
MESSAGE("${str}enabled")
ELSE(${USE} AND ${FOUND})
MESSAGE("${str}disabled")
ENDIF(${USE} AND ${FOUND})
ENDMACRO(PRINT_SUMMARY)

IF (UNIX)
ADD_CUSTOM_TARGET (distclean @echo cleaning for source distribution)

ADD_CUSTOM_COMMAND(
    COMMENT "distribution clean"
    COMMAND make
    ARGS -C ${CMAKE_CURRENT_BINARY_DIR} clean
    COMMAND find
    ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "CMakeCache.txt" | xargs rm -rf
    COMMAND find
    ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "CMakeFiles" | xargs rm -rf
    COMMAND find
    ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "Makefile" | xargs rm -rf
    COMMAND find
    ARGS ${CMAKE_CURRENT_BINARY_DIR} -name \"*.cmake\" | xargs rm -rf
    COMMAND find
    ARGS ${CMAKE_CURRENT_SOURCE_DIR} -name "*.qm" | xargs rm -rf
    COMMAND find
    ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "*.pc" | xargs rm -rf
    COMMAND find
    ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "*.depends" | xargs rm -rf
    COMMAND find
    ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "*.cxx_parameters" | xargs rm -rf
    COMMAND find
    ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "moc_*.cpp" | xargs rm -rf
    COMMAND find
    ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "qrc_*.cxx" | xargs rm -rf
    COMMAND find
    ARGS ${CMAKE_CURRENT_BINARY_DIR} -name "*_automoc.cpp" | xargs rm -rf
    COMMAND rm
    ARGS -rf ${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt
    COMMAND find
    ARGS ${CMAKE_CURRENT_BINARY_DIR} -name *_automoc.dir | xargs rm -rf
    COMMAND rm
    ARGS -rf ${CMAKE_CURRENT_BINARY_DIR}/doc/html
    TARGET  distclean
)
ENDIF(UNIX)


MESSAGE("")
MESSAGE("Transports:")
PRINT_SUMMARY ("HTTP support .........................." USE_CURL CURL_FOUND)
PRINT_SUMMARY ("MMS support ..........................." USE_MMS MMS_FOUND)

MESSAGE("")
MESSAGE("Input Plugins:")
PRINT_SUMMARY ("MPEG support .........................." 1 MAD_FOUND OR MPG123_FOUND)
PRINT_SUMMARY ("FLAC support .........................." USE_FLAC FLAC_FOUND)
PRINT_SUMMARY ("Ogg Vorbis support ...................." USE_VORBIS VORBIS_FOUND)
PRINT_SUMMARY ("Musepack support ......................" USE_MPC MPC_FOUND)
PRINT_SUMMARY ("MOD support ..........................." USE_MODPLUG MODPLUG_FOUND)
PRINT_SUMMARY ("Wave support .........................." USE_SNDFILE SNDFILE_FOUND)
PRINT_SUMMARY ("WavPack support ......................." USE_WAVPACK WAVPACK_FOUND)
PRINT_SUMMARY ("FFmpeg support ........................" USE_FFMPEG FFMPEG_FOUND)
PRINT_SUMMARY ("AAC support ..........................." USE_AAC FAAD_FOUND)
PRINT_SUMMARY ("CUE sheet support ....................." USE_CUE 1)
PRINT_SUMMARY ("Mplayer support ......................." USE_MPLAYER 1)
PRINT_SUMMARY ("CD Audio support ......................" USE_CDA CDIO_FOUND)
PRINT_SUMMARY ("Midi support .........................." USE_MIDI WILDMIDI_FOUND)
PRINT_SUMMARY ("Game music support ...................." USE_GME GME_FOUND)
PRINT_SUMMARY ("Ogg Opus support ......................" USE_OPUS OPUS_FOUND)
PRINT_SUMMARY ("SID support ..........................." USE_SID SIDPLAYFP_FOUND)
PRINT_SUMMARY ("Archive support ......................." USE_ARCHIVE LIBARCHIVE_FOUND)

MESSAGE("")
MESSAGE("Output Plugins:")
PRINT_SUMMARY ("ALSA support .........................." USE_ALSA ALSA_FOUND)
PRINT_SUMMARY ("OSS support ..........................." USE_OSS 1)
PRINT_SUMMARY ("OSS4 support .........................." USE_OSS4 OSS4_FOUND)
PRINT_SUMMARY ("JACK support .........................." USE_JACK JACK_FOUND)
PRINT_SUMMARY ("PulseAudio support ...................." USE_PULSE PULSE_FOUND)
PRINT_SUMMARY ("Null output support ..................." USE_NULL 1)
PRINT_SUMMARY ("Waveout support (Windows) ............." USE_WAVEOUT WINDOWS_H_FOUND)
PRINT_SUMMARY ("DirectSound support (Windows) ........." USE_DSOUND DSOUND_H_FOUND)
PRINT_SUMMARY ("WASAPI support (Windows) .............." USE_WASAPI WASAPI_FOUND)
PRINT_SUMMARY ("Qt Multimedia support ................." USE_QTMULTIMEDIA Qt5Multimedia_FOUND)
PRINT_SUMMARY ("Icecast support ......................." USE_SHOUT SHOUT_FOUND)

MESSAGE("")
MESSAGE("Effect Plugins:")
PRINT_SUMMARY ("SoX Resampler ........................." USE_SOXR SOXR_FOUND)
PRINT_SUMMARY ("BS2B Plugin ..........................." USE_BS2B BS2B_FOUND)
PRINT_SUMMARY ("LADSPA Host ..........................." USE_LADSPA 1)
PRINT_SUMMARY ("Crossfade ............................." USE_CROSSFADE 1)
PRINT_SUMMARY ("Extra Stereo .........................." USE_STEREO 1)
PRINT_SUMMARY ("File Writer ..........................." USE_FILEWRITER VORBISENC_FOUND)
PRINT_SUMMARY ("Mono to Stereo Converter .............." USE_MONOTOSTEREO 1)

MESSAGE("")
MESSAGE("Visual Plugins:")
PRINT_SUMMARY ("Analyzer .............................." USE_ANALYZER 1)
PRINT_SUMMARY ("ProjectM .............................." USE_PROJECTM PROJECTM_FOUND)

MESSAGE("")
MESSAGE("General Plugins:")
PRINT_SUMMARY ("MPRIS support ........................." USE_MPRIS Qt5DBus_FOUND)
PRINT_SUMMARY ("Scrobbler ............................." USE_SCROBBLER 1)
PRINT_SUMMARY ("ListenBrainz .........................." USE_LISTENBRAINZ 1)
PRINT_SUMMARY ("Tray support .........................." USE_STATICON 1)
PRINT_SUMMARY ("Notifier support ......................" USE_NOTIFIER 1)
PRINT_SUMMARY ("Lyrics support ........................" USE_LYRICS 1)
PRINT_SUMMARY ("HAL support ..........................." USE_HAL Qt5DBus_FOUND)
PRINT_SUMMARY ("UDisks2 support ......................." USE_UDISKS2 Qt5DBus_FOUND)
PRINT_SUMMARY ("Global hotkey support ................." USE_HOTKEY X11_FOUND AND XCB_H_FOUND AND Qt5X11Extras_FOUND)
PRINT_SUMMARY ("GNOME/Cinnamon hotkey support ........." USE_GNOMEHOTKEY Qt5DBus_FOUND)
PRINT_SUMMARY ("File operations ......................." USE_FILEOPS 1)
PRINT_SUMMARY ("Cover manager ........................." USE_COVER 1)
PRINT_SUMMARY ("KDE notifications support ............." USE_KDENOTIFY Qt5DBus_FOUND)
PRINT_SUMMARY ("Audio converter ......................." USE_CONVERTER TAGLIB_FOUND)
PRINT_SUMMARY ("ReplayGain Scanner ...................." USE_RGSCAN TAGLIB_FOUND)
PRINT_SUMMARY ("Stream browser ........................" USE_SB 1)
PRINT_SUMMARY ("Track change .........................." USE_TRACKCHANGE 1)
PRINT_SUMMARY ("Copy/Paste ............................" USE_COPYPASTE 1)
PRINT_SUMMARY ("History plugin ........................" USE_HISTORY Qt5Sql_FOUND)
PRINT_SUMMARY ("Sleep mode inhibition plugin .........." USE_SLEEPINHIBITOR Qt5DBus_FOUND)
PRINT_SUMMARY ("Taskbar plugin (Windows) .............." USE_TASKBAR Qt5WinExtras_FOUND)
PRINT_SUMMARY ("Removable device detection (Windows) .." USE_RDETECT Qt5WinExtras_FOUND)

MESSAGE("")
MESSAGE("File Dialogs:")
PRINT_SUMMARY ("QMMP File Dialog ......................" USE_QMMP_DIALOG 1)
PRINT_SUMMARY ("Two-panel File File Dialog ............" USE_TWO_PANEL_DIALOG 1)

MESSAGE("")
MESSAGE("Advanced:")
PRINT_SUMMARY ("Skinned user interface ................" USE_SKINNED X11_FOUND AND Qt5X11Extras_FOUND)
PRINT_SUMMARY ("Simple user interface (QSUI) .........." USE_QSUI 1)
PRINT_SUMMARY ("Automatic charset detection ..........." USE_ENCA ENCA_FOUND)
PRINT_SUMMARY ("Directory association ................." USE_DIR_ASSOC 1)
PRINT_SUMMARY ("Set RPATH in installed binaries ......." CMAKE_INSTALL_RPATH_USE_LINK_PATH 1)
IF(MPEG_DECODERS)
MESSAGE       ("MPEG decoders ........................." "${MPEG_DECODERS}")
ENDIF(MPEG_DECODERS)

MESSAGE("")
