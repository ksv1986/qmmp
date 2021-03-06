project(libwavpack)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# libqmmp
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../qmmp)

# wavpack
pkg_search_module(WAVPACK wavpack)

include_directories(${WAVPACK_INCLUDE_DIRS})
link_directories(${WAVPACK_LIBRARY_DIRS})
ADD_DEFINITIONS(${WAVPACK_CLAGS})

SET(libwavpack_SRCS
  decoder_wavpack.cpp
  decoderwavpackfactory.cpp
  wavpackmetadatamodel.cpp
)

SET(libwavpack_HDRS
  decoder_wavpack.h
)

SET(libwavpack_RCCS translations/translations.qrc)

QT5_ADD_RESOURCES(libwavpack_RCC_SRCS ${libwavpack_RCCS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

IF(WAVPACK_FOUND)
ADD_LIBRARY(wavpack MODULE ${libwavpack_SRCS} ${libwavpack_RCC_SRCS} ${libwavpack_HDRS})
add_dependencies(wavpack libqmmp)
target_link_libraries(wavpack Qt5::Widgets libqmmp ${WAVPACK_LDFLAGS})
install(TARGETS wavpack DESTINATION ${PLUGIN_DIR}/Input)
ENDIF(WAVPACK_FOUND)

