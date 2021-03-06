project(libanalyzer)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# libqmmp
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../qmmp)

SET(libanalyzer_SRCS
    analyzer.cpp
    colorwidget.cpp
    settingsdialog.cpp
    visualanalyzerfactory.cpp
    fft.c
)

SET(libanalyzer_HDRS
   inlines.h
   fft.h
)

SET(libanalyzer_RCCS translations/translations.qrc)

QT5_ADD_RESOURCES(libanalyzer_RCC_SRCS ${libanalyzer_RCCS})

# user interface


SET(libanalyzer_UIS
    settingsdialog.ui
)

QT5_WRAP_UI(libanalyzer_UIS_H ${libanalyzer_UIS})
# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(analyzer MODULE ${libanalyzer_SRCS} ${libanalyzer_UIS_H}
  ${libanalyzer_RCC_SRCS} ${libanalyzer_HDRS})
add_dependencies(analyzer libqmmp)
target_link_libraries(analyzer Qt5::Widgets libqmmp)
install(TARGETS analyzer DESTINATION ${PLUGIN_DIR}/Visual)
