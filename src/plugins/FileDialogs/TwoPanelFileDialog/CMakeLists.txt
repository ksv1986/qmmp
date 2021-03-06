project(libtwopanelfiledialog)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# libqmmpui
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../qmmpui)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../qmmp)

SET(libtwopanelfiledialog_SRCS
    twopanelfiledialogimpl.cpp
    twopanelfiledialog.cpp
)

SET(libtwopanelfiledialog_RCCS
    translations/translations.qrc)

QT5_ADD_RESOURCES(libtwopanelfiledialog_RCC_SRCS ${libtwopanelfiledialog_RCCS})

# user interface


SET(libtwopanelfiledialog_UIS
    twopanelfiledialog.ui
)

QT5_WRAP_UI(libtwopanelfiledialog_UIS_H ${libtwopanelfiledialog_UIS})
# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(twopanelfiledialog MODULE ${libtwopanelfiledialog_SRCS} ${libtwopanelfiledialog_UIS_H} ${libtwopanelfiledialog_RCC_SRCS})
add_dependencies(twopanelfiledialog libqmmpui)
target_link_libraries(twopanelfiledialog Qt5::Widgets libqmmpui libqmmp)
install(TARGETS twopanelfiledialog DESTINATION ${PLUGIN_DIR}/FileDialogs)
