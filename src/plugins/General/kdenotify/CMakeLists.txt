project(libkdenotify)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# libqmmpui & libqmmp
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../qmmpui)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../qmmp)

SET(libkdenotify_SRCS
    kdenotify.cpp
    kdenotifyfactory.cpp
    settingsdialog.cpp
)

SET(libkdenotify_RCCS translations/translations.qrc)

QT5_ADD_RESOURCES(libkdenotify_RCC_SRCS ${libkdenotify_RCCS})

# user interface
SET(libkdenotify_UIS
    settingsdialog.ui
)

QT5_WRAP_UI(libkdenotify_UIS_H ${libkdenotify_UIS})

# QT5_WRAP_UI(libkdenotify_UIS_H ${libkdenotify_UIS})
# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(kdenotify MODULE ${libkdenotify_SRCS} ${libkdenotify_RCC_SRCS} ${libkdenotify_UIS_H})
add_dependencies(kdenotify libqmmpui)
target_link_libraries(kdenotify Qt5::Widgets Qt5::DBus libqmmpui libqmmp)
install(TARGETS kdenotify DESTINATION ${PLUGIN_DIR}/General)
install(FILES images/empty_cover.png DESTINATION ${CMAKE_INSTALL_DATADIR}/qmmp${APP_NAME_SUFFIX}/images)
install(FILES images/app_icon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/qmmp${APP_NAME_SUFFIX}/images)
