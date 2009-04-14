cmake_minimum_required(VERSION 2.6)
#set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules )

##  global setup
project(libcamiface)

# version
set(V_MAJOR 0)
set(V_MINOR 5)
set(V_PATCH 2)

include_directories( ${CMAKE_SOURCE_DIR}/inc )

FIND_PACKAGE(PkgConfig)

pkg_check_modules(DC1394 libdc1394-2)

#FIND_PACKAGE(DC1394)
#INCLUDE_DIRECTORIES(${DC1394_INCLUDE_PATH})

# create config.h file
#configure_file(src/camiface_config.h.in.cmake ${CMAKE_BINARY_DIR}/src/camiface_config.h)

set(all_backends unity)

IF(DC1394_FOUND)
  set(all_backends ${all_backends} dc1394)
ENDIF(DC1394_FOUND)


add_subdirectory(src)
add_subdirectory(demo)

SET(CPACK_PACKAGE_VERSION_MAJOR "${V_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${V_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${V_PATCH}")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${V_MAJOR}.${V_MINOR}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "libcamiface-${V_MAJOR}.${V_MINOR}.${V_PATCH}")
SET(CPACK_SOURCE_GENERATOR "TGZ")
