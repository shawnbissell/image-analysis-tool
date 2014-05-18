cmake_minimum_required (VERSION 2.6)
project (image-tools)

include(ExternalProject)
ExternalProject_Add(psol
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/download
    URL https://dl.google.com/dl/page-speed/psol/1.8.31.2.tar.gz
    URL_MD5 efa3fe52226f15827ffff50537dde368
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/psol
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Add(libwebp
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/download
    URL https://webp.googlecode.com/files/libwebp-0.4.0.tar.gz
    URL_MD5 c8dd1d26eb9566833aba269b86d97e68
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/libwebp
    CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/libwebp/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND ${MAKE}
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)


include_directories(${CMAKE_CURRENT_BINARY_DIR}/psol/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/psol/include/third_party/chromium/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/psol/include/pagespeed/kernel)


add_executable(imgt ImageTools.cc Image.cc)

add_dependencies(imgt libwebp)
add_dependencies(imgt psol)

target_link_libraries(imgt ${CMAKE_CURRENT_BINARY_DIR}/psol/lib/Release/linux/x64/pagespeed_automatic.a pthread ${PROJECT_BINARY_DIR}/libwebp/src/.libs/libwebp.a)