# demo programs with no compile/link needs beyond libcamiface
set(SRCS
    multi-cam-autoset
    multi-cam
    simple
    )

set(DEMO_DIR bin)

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

  set(glut_SRCS liveview-glut.c)

  IF(GLEW_FOUND)
    SET_SOURCE_FILES_PROPERTIES(
      ${glut_SRCS}
      PROPERTIES COMPILE_FLAGS "-DUSE_GLEW")
  ELSE(GLEW_FOUND)
    SET(GLEW_INCLUDE_DIR "")
    SET(GLEW_LIBRARY "")
    message("GLEW not found -- no OpenGL extension support in liveview-glut")
  ENDIF(GLEW_FOUND)

  add_library(imp_glew STATIC IMPORTED)
  set_property(TARGET imp_glew PROPERTY
               IMPORTED_LOCATION "${GLEW_LIBRARY}")

  FOREACH(BACKEND ${all_backends})
    ADD_EXECUTABLE(liveview-glut-${BACKEND} ${glut_SRCS})
    include_directories(${GLUT_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})
    target_link_libraries(liveview-glut-${BACKEND} imp_glew)

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
ELSE(GLUT_FOUND)
  message("GLUT not found -- no liveview-glut")
ENDIF(GLUT_FOUND)
