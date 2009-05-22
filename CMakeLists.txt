cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules )

##  global setup
project(libcamiface)

# version
set(V_MAJOR 0)
set(V_MINOR 5)
set(V_PATCH 3)

include_directories( ${CMAKE_SOURCE_DIR}/inc )

FIND_PACKAGE(PkgConfig)

# Backend: unity will always be built. ---------------
if(NOT WIN32)
set(all_backends unity)
endif(NOT WIN32)

# Backend: libdc1394 ---------------------------------

# Method A: Check for libdc1394 using pkg-config (preferred)
pkg_check_modules(DC1394 libdc1394-2)

# Method B: Check for libdc1394 ourselves without pkg-config (e.g. on Mac)
IF(NOT DC1394_FOUND)
  FIND_PACKAGE(DC1394)
ENDIF(NOT DC1394_FOUND)

IF(DC1394_FOUND)
  set(all_backends ${all_backends} dc1394)
ENDIF(DC1394_FOUND)

# Backend: Prosilica --------------------------------

FIND_PACKAGE(ProsilicaGigE)

IF(PROSILICA_GIGE_FOUND)
  set(all_backends ${all_backends} prosilica_gige)
ENDIF(PROSILICA_GIGE_FOUND)

# Backend: QuickTime --------------------------------

FIND_PACKAGE(QuickTime)
FIND_PACKAGE(Carbon)

IF(QUICKTIME_FOUND)
  set(all_backends ${all_backends} quicktime)
ENDIF(QUICKTIME_FOUND)

# Build targets -------------------------------------

# create config.h file
#configure_file(src/camiface_config.h.in.cmake ${CMAKE_BINARY_DIR}/src/camiface_config.h)

add_subdirectory(src)
add_subdirectory(demo)

# Install header ------------------------------------

INSTALL(FILES inc/cam_iface.h
  DESTINATION include)

# Create pkg-config file

SET(PKG_CONFIG_VERSION "${V_MAJOR}.${V_MINOR}.${V_PATCH}")
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir      ${CMAKE_INSTALL_PREFIX}/lib)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)

configure_file(libcamiface.pc.in libcamiface.pc)

# CPack for binary and source releases --------------

# For binary releases

SET(CPACK_PACKAGE_NAME "libcamiface")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "libcamiface - camera interface library")
SET(CPACK_PACKAGE_VENDOR "Motmot authors")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.rst")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

SET(CPACK_PACKAGE_VERSION_MAJOR "${V_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${V_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${V_PATCH}")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "libcamiface ${V_MAJOR}.${V_MINOR}.${V_PATCH}")

# For source releases (except we use git archive to make these)

SET(CPACK_SOURCE_PACKAGE_FILE_NAME 
    "libcamiface-${V_MAJOR}.${V_MINOR}.${V_PATCH}")
SET(CPACK_SOURCE_GENERATOR "TGZ")

INCLUDE(CPack)
