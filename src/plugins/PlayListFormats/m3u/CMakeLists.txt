project(libm3uplaylistformat)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# libqmmpui
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../qmmpui)

SET(libm3uplaylistformat_SRCS
    m3uplaylistformat.cpp
)

QT5_ADD_RESOURCES(libm3uplaylistformat_RCC_SRCS ${libm3uplaylistformat_RCCS})

# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(m3uplaylistformat MODULE ${libm3uplaylistformat_SRCS})

add_dependencies(m3uplaylistformat libqmmpui)
target_link_libraries(m3uplaylistformat Qt5::Widgets libqmmpui)
install(TARGETS m3uplaylistformat DESTINATION ${PLUGIN_DIR}/PlayListFormats)
