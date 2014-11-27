cmake_minimum_required (VERSION 2.6)
project (image-analysis-tool)

set (ImageAnalysisTool_VERSION_MAJOR 0)
set (ImageAnalysisTool_VERSION_MINOR 3)
configure_file (
  "${PROJECT_SOURCE_DIR}/ImageAnalysisToolConfig.h.in"
  "${PROJECT_BINARY_DIR}/ImageAnalysisToolConfig.h"
  )

include(ExternalProject)
ExternalProject_Add(psol
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/download
    URL http://dl.google.com/dl/page-speed/psol/1.8.31.2.tar.gz
    URL_MD5 efa3fe52226f15827ffff50537dde368
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/psol
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Add(libwebp
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/download
    URL http://webp.googlecode.com/files/libwebp-0.4.0.tar.gz
    URL_MD5 c8dd1d26eb9566833aba269b86d97e68
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/libwebp
    CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/libwebp/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND ${MAKE}
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)

ExternalProject_Add(giflib
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/download
    URL http://sourceforge.net/projects/giflib/files/giflib-5.1.0.tar.gz
    URL_MD5 40248cb52f525dc82981761628dbd853
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/giflib
    CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/giflib/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND ${MAKE}
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/psol/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/psol/include/third_party/chromium/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/psol/include/pagespeed/kernel)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/libwebp/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/giflib/lib)


add_executable(imgat ImageAnalysisTool.cc Image.cc)

add_dependencies(imgat libwebp)
add_dependencies(imgat giflib)
add_dependencies(imgat psol)

target_link_libraries(imgat ${CMAKE_CURRENT_BINARY_DIR}/psol/lib/Release/linux/x64/pagespeed_automatic.a 
                            pthread rt
                            ${PROJECT_BINARY_DIR}/libwebp/src/.libs/libwebp.a
                            ${PROJECT_BINARY_DIR}/giflib/lib/.libs/libgif.a
)

install(TARGETS imgat
        RUNTIME DESTINATION bin
)