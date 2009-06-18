# Copyright (c) 2004-2009, California Institute of Technology. All
# rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:

#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.

#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# demo programs with no compile/link needs beyond libcamiface
set(SRCS
    multi-cam-autoset
    multi-cam
    simple
    )

set(DEMO_DIR bin)
IF(UNIX AND NOT APPLE)
  set(USR_SHARE_DIR "${CMAKE_INSTALL_PREFIX}/share/libcamiface/")
  SET(SHADER_SOURCE_DIR ${USR_SHARE_DIR})
ELSE(UNIX AND NOT APPLE)
  SET(SHADER_SOURCE_DIR "./")
ENDIF(UNIX AND NOT APPLE)

FOREACH(SRC ${SRCS})
  FOREACH(BACKEND ${all_backends})
    ADD_EXECUTABLE(${SRC}-${BACKEND} ${SRC}.c)
    TARGET_LINK_LIBRARIES(${SRC}-${BACKEND} cam_iface_${BACKEND})

    INSTALL(TARGETS ${SRC}-${BACKEND}
      RUNTIME DESTINATION ${DEMO_DIR}
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
    )
  ENDFOREACH(BACKEND)
ENDFOREACH(SRC ${SRCS})

# demo programs with more complex compile/link needs

#  opencv.c
#   Check for opencv using pkg-config
pkg_check_modules(OPENCV opencv)

if(OPENCV_FOUND)

  set(opencv_SRCS opencv.c)

  FOREACH(BACKEND ${all_backends})
    ADD_EXECUTABLE(opencv-${BACKEND} ${opencv_SRCS})
    include_directories(${OPENCV_INCLUDE_DIRS})

    TARGET_LINK_LIBRARIES(opencv-${BACKEND}
      cam_iface_${BACKEND}
      ${OPENCV_LIBRARIES}
      )

    INSTALL(TARGETS opencv-${BACKEND}
      RUNTIME DESTINATION ${DEMO_DIR}
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      )
  ENDFOREACH(BACKEND)
ENDIF(OPENCV_FOUND)

#  cam2theora.c
#   Check for libtheora using pkg-config
pkg_check_modules(THEORA theora)

if(THEORA_FOUND)

  set(cam2theora_SRCS cam2theora.c)

  FOREACH(BACKEND ${all_backends})
    ADD_EXECUTABLE(cam2theora-${BACKEND} ${cam2theora_SRCS})
    include_directories(${THEORA_INCLUDE_DIRS})

    TARGET_LINK_LIBRARIES(cam2theora-${BACKEND}
      cam_iface_${BACKEND}
      ${THEORA_LIBRARIES}
      )

    INSTALL(TARGETS cam2theora-${BACKEND}
      RUNTIME DESTINATION ${DEMO_DIR}
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      )
  ENDFOREACH(BACKEND)

# temporary: png2theora
  pkg_check_modules(LIBPNG libpng)

  set(png2theora_SRCS png2theora.c)

  ADD_EXECUTABLE(png2theora ${png2theora_SRCS})
  include_directories(${THEORA_INCLUDE_DIRS} ${LIBPNG_INCLUDE_DIRS})

  TARGET_LINK_LIBRARIES(png2theora
    ${THEORA_LIBRARIES}
    ${LIBPNG_LIBRARIES}
    )

  INSTALL(TARGETS png2theora
    RUNTIME DESTINATION ${DEMO_DIR}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

ENDIF(THEORA_FOUND)

#  glut.c

IF(WIN32)
  # Download from http://www.xmission.com/~nate/glut.html.  Then
  # install glut.h in GLUT_ROOT_PATH\include\GL and the rest in
  # GLUT_ROOT_PATH\Release.
  SET(GLUT_ROOT_PATH "C:/Program\ Files/GLUT32")
ENDIF(WIN32)

SET(GLEW_ROOT "$ENV{GLEW_ROOT}")
IF(GLEW_ROOT STREQUAL "")
  # Use FindGLEW.cmake recipe
  FIND_PACKAGE(GLEW)
ELSE(GLEW_ROOT STREQUAL "")
  # Set values based on GLEW_ROOT
  SET(GLEW_FOUND TRUE)
  SET(GLEW_INCLUDE_DIR "${GLEW_ROOT}/include")
  SET(GLEW_LIBRARY "${GLEW_ROOT}/lib/libGLEW.a")
ENDIF(GLEW_ROOT STREQUAL "")
FIND_PACKAGE(GLUT)
FIND_PACKAGE(OpenGL)

if(GLUT_FOUND)

  set(liveview-glut-SRCS liveview-glut.c)
  set(liveview_DEFINE "-DSHADER_DIR='\"${SHADER_SOURCE_DIR}\"'"
    ${liveview_DEFINE})

  IF(GLEW_FOUND)
    set(liveview_DEFINE "-DUSE_GLEW"
      ${liveview_DEFINE})

    add_library(imp_glew STATIC IMPORTED)
    set_property(TARGET imp_glew PROPERTY
      IMPORTED_LOCATION "${GLEW_LIBRARY}")

  ELSE(GLEW_FOUND)
    SET(GLEW_INCLUDE_DIR "")
    SET(GLEW_LIBRARY "")
    message("GLEW not found -- no OpenGL extension support in liveview-glut")
  ENDIF(GLEW_FOUND)

  # concatenate (join) strings to make single argument
  FOREACH(element ${liveview_DEFINE})
    set(liveview_DEFINES "${liveview_DEFINES} ${element}")
  ENDFOREACH(element)

  SET_SOURCE_FILES_PROPERTIES(
    ${liveview-glut-SRCS}
    PROPERTIES COMPILE_FLAGS ${liveview_DEFINES})

  FOREACH(BACKEND ${all_backends})
    ADD_EXECUTABLE(liveview-glut-${BACKEND} ${liveview-glut-SRCS})
    include_directories(${GLUT_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})
    IF(GLEW_FOUND)
      target_link_libraries(liveview-glut-${BACKEND} imp_glew)
    ENDIF(GLEW_FOUND)

    TARGET_LINK_LIBRARIES(liveview-glut-${BACKEND}
      cam_iface_${BACKEND}
      ${GLUT_LIBRARIES}
      ${OPENGL_LIBRARIES}
      )

    INSTALL(TARGETS liveview-glut-${BACKEND}
      RUNTIME DESTINATION ${DEMO_DIR}
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      )

    IF(WIN32)
      GET_FILENAME_COMPONENT(GLUT_LIB_DIR ${GLUT_LIBRARIES} PATH)
      FILE(GLOB GLUT_DLLS ${GLUT_LIB_DIR}/*.dll)
      IF(GLEW_FOUND)
        FILE(GLOB GLEW_DLLS ${GLEW_LIB_DIR}/*.dll)
      ENDIF(GLEW_FOUND)
      INSTALL(FILES ${GLUT_DLLS} ${GLEW_DLLS} DESTINATION ${DEMO_DIR})
    ENDIF(WIN32)

  ENDFOREACH(BACKEND)

  IF(UNIX AND NOT APPLE)
    # Install shaders in /usr/share equivalent for FHS
    INSTALL(FILES "demosaic.vrt" "demosaic.frg" DESTINATION ${USR_SHARE_DIR})
  ELSE(UNIX AND NOT APPLE)
    # Install shaders alongside demos
    INSTALL(FILES "demosaic.vrt" "demosaic.frg" DESTINATION ${DEMO_DIR})
  ENDIF(UNIX AND NOT APPLE)

ELSE(GLUT_FOUND)
  message("GLUT not found -- no liveview-glut")
ENDIF(GLUT_FOUND)
