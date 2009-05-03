# demo programs with no compile/link needs beyond libcamiface
set(SRCS
    multi-cam-autoset
    multi-cam
    simple
    )

FOREACH(SRC ${SRCS})
  FOREACH(BACKEND ${all_backends})
    ADD_EXECUTABLE(${SRC}-${BACKEND} ${SRC}.c)
    TARGET_LINK_LIBRARIES(${SRC}-${BACKEND} cam_iface_${BACKEND})

    INSTALL(TARGETS ${SRC}-${BACKEND}
      RUNTIME DESTINATION bin
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
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      )
  ENDFOREACH(BACKEND)
ENDIF(OPENCV_FOUND)
