set(common_SRCS
    cam_iface_common.c
    )

set(CAM_IFACE_VERSION "${V_MAJOR}.${V_MINOR}.${V_PATCH}")
set(CAM_IFACE_SOVERSION 0)

# unity backend -----------------

set(unity_SRCS ${common_SRCS}
    cam_iface_unity.c
    )

# get list of all (non-unity) backends to wrap into unity backend
FOREACH(BACKEND ${all_backends})
  IF(${BACKEND} STREQUAL "unity")
    # don't include unity backend in list of backends for unity
  ELSE(${BACKEND} STREQUAL "unity")
    set(INDIVIDUAL_UNITY_BACKENDS ${INDIVIDUAL_UNITY_BACKENDS} ${BACKEND})
  ENDIF(${BACKEND} STREQUAL "unity")
ENDFOREACH(BACKEND)

FOREACH(BACKEND ${INDIVIDUAL_UNITY_BACKENDS})
  IF(DEFINED UNITY_BACKENDS)
    set(UNITY_BACKENDS "${UNITY_BACKENDS},\"${BACKEND}\"")
  ELSE(DEFINED UNITY_BACKENDS)
    set(UNITY_BACKENDS "\"${BACKEND}\"")
  ENDIF(DEFINED UNITY_BACKENDS)
ENDFOREACH(BACKEND)

list(LENGTH INDIVIDUAL_UNITY_BACKENDS NUM_BACKENDS)

MESSAGE("unity backend includes ${NUM_BACKENDS} other backends: (${UNITY_BACKENDS})")

# list of each command-line define
set(unity_DEFINE
    -DNUM_BACKENDS=${NUM_BACKENDS}
    -DUNITY_BACKENDS='{${UNITY_BACKENDS}}'
    -DUNITY_BACKEND_PREFIX='\"lib\"'
    -DUNITY_BACKEND_SUFFIX='\".so\"'
    -DUNITY_BACKEND_DIR='\"\"')

# concatenate (join) strings to make single argument
FOREACH(element ${unity_DEFINE})
  set(unity_DEFINES "${unity_DEFINES} ${element}")
ENDFOREACH(element ${unity_DEFINE})

SET_SOURCE_FILES_PROPERTIES(
    cam_iface_unity.c
    PROPERTIES COMPILE_FLAGS ${unity_DEFINES})

ADD_LIBRARY(cam_iface_unity SHARED ${unity_SRCS})
TARGET_LINK_LIBRARIES(cam_iface_unity dl)
set_target_properties(cam_iface_unity PROPERTIES VERSION ${CAM_IFACE_VERSION}
  SOVERSION ${CAM_IFACE_SOVERSION})

ADD_LIBRARY(cam_iface_unity-static STATIC ${unity_SRCS})
set_target_properties(cam_iface_unity-static PROPERTIES OUTPUT_NAME "cam_iface_unity")

# These next two lines are required but it is unclear exactly what they do.
# The CMake FAQ mentions they are necessary and it does not work otherwise.

SET_TARGET_PROPERTIES(cam_iface_unity PROPERTIES CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(cam_iface_unity-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

INSTALL(TARGETS cam_iface_unity
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

# dc1394 backend ------------------

IF(DC1394_FOUND)

  set(dc1394_SRCS ${common_SRCS}
      cam_iface_dc1394.c
      )

  ADD_LIBRARY(cam_iface_dc1394 SHARED ${dc1394_SRCS})
  TARGET_LINK_LIBRARIES(cam_iface_dc1394 ${DC1394_LIBRARIES})
  set_target_properties(cam_iface_dc1394 PROPERTIES VERSION ${CAM_IFACE_VERSION}
    SOVERSION ${CAM_IFACE_SOVERSION})

  ADD_LIBRARY(cam_iface_dc1394-static STATIC ${dc1394_SRCS})
  set_target_properties(cam_iface_dc1394-static PROPERTIES OUTPUT_NAME "cam_iface_dc1394")

  SET_TARGET_PROPERTIES(cam_iface_dc1394 PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  SET_TARGET_PROPERTIES(cam_iface_dc1394-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

  INSTALL(TARGETS cam_iface_dc1394
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

ENDIF(DC1394_FOUND)

# prosilica_gige backend ------------------

IF(PROSILICA_GIGE_FOUND)

  set(prosilica_gige_SRCS ${common_SRCS}
      cam_iface_prosilica_gige.cpp
      )

  # list of each command-line define
  IF(WIN32)
    set(prosilica_gige_DEFINE
        -D_x32
        -D_WINDOWS
        )
  ELSE(WIN32)
  # XXX these need to be improved for multi-arch support (and go to
  #  compile-time test for 32 vs 64 bit)
    set(prosilica_gige_DEFINE
        -D_x64
        -D_LINUX
        )
  ENDIF(WIN32)

  # concatenate (join) strings to make single argument
  FOREACH(element ${prosilica_gige_DEFINE})
    set(prosilica_gige_DEFINES "${prosilica_gige_DEFINES} ${element}")
  ENDFOREACH(element)

  SET_SOURCE_FILES_PROPERTIES(
      cam_iface_prosilica_gige.cpp
      PROPERTIES COMPILE_FLAGS ${prosilica_gige_DEFINES})

  include_directories(${PROSILICA_GIGE_INCLUDE_DIRS})
  ADD_LIBRARY(cam_iface_prosilica_gige SHARED ${prosilica_gige_SRCS})

  TARGET_LINK_LIBRARIES(cam_iface_prosilica_gige ${PROSILICA_GIGE_LIBRARIES})
  set_target_properties(cam_iface_prosilica_gige PROPERTIES VERSION ${CAM_IFACE_VERSION}
    SOVERSION ${CAM_IFACE_SOVERSION})

  ADD_LIBRARY(cam_iface_prosilica_gige-static STATIC ${prosilica_gige_SRCS})
  set_target_properties(cam_iface_prosilica_gige-static PROPERTIES OUTPUT_NAME "cam_iface_prosilica_gige")

  SET_TARGET_PROPERTIES(cam_iface_prosilica_gige PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  SET_TARGET_PROPERTIES(cam_iface_prosilica_gige-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

  INSTALL(TARGETS cam_iface_prosilica_gige
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

ENDIF(PROSILICA_GIGE_FOUND)

# quicktime backend ------------------

IF(QUICKTIME_FOUND)

IF(NOT CARBON_FOUND)
  MESSAGE(FATAL_ERROR "QuickTime backend depends on Carbon, but Carbon not found")
ENDIF(NOT CARBON_FOUND)

  set(quicktime_SRCS ${common_SRCS}
      cam_iface_quicktime.c
      )

  ADD_LIBRARY(cam_iface_quicktime SHARED ${quicktime_SRCS})
  TARGET_LINK_LIBRARIES(cam_iface_quicktime ${QUICKTIME_LIBRARY} ${CARBON_LIBRARY})
  set_target_properties(cam_iface_quicktime PROPERTIES VERSION ${CAM_IFACE_VERSION}
    SOVERSION ${CAM_IFACE_SOVERSION})

  ADD_LIBRARY(cam_iface_quicktime-static STATIC ${quicktime_SRCS})
  set_target_properties(cam_iface_quicktime-static PROPERTIES OUTPUT_NAME "cam_iface_quicktime")

  SET_TARGET_PROPERTIES(cam_iface_quicktime PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  SET_TARGET_PROPERTIES(cam_iface_quicktime-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

  INSTALL(TARGETS cam_iface_quicktime
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

ENDIF(QUICKTIME_FOUND)
