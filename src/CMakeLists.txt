set(common_SRCS
    cam_iface_common.c
    )

# unity backend -----------------

set(unity_SRCS ${common_SRCS}
    cam_iface_unity.c
    )

# get list of all (non-unity) backends to wrap into unity backend
FOREACH(BACKEND ${all_backends})
  IF(${BACKEND} STREQUAL "unity")
    # don't include unity backend in list of backends for unity
  ELSE(${BACKEND} STREQUAL "unity")
    IF(DEFINED UNITY_BACKENDS)
      set(UNITY_BACKENDS "${UNITY_BACKENDS},\"${BACKEND}\"")
    ELSE(DEFINED UNITY_BACKENDS)
      set(UNITY_BACKENDS "\"${BACKEND}\"")
    ENDIF(DEFINED UNITY_BACKENDS)
  ENDIF(${BACKEND} STREQUAL "unity")
ENDFOREACH(BACKEND)

list(LENGTH UNITY_BACKENDS NUM_BACKENDS)

MESSAGE("NUM BACKENDS: ${NUM_BACKENDS}")

# get list of each command-line define
set(unity_DEFINE
    -DNUM_BACKENDS=${NUM_BACKENDS}
    -DUNITY_BACKENDS='{${UNITY_BACKENDS}}'
    -DUNITY_BACKEND_PREFIX='\"lib\"'
    -DUNITY_BACKEND_SUFFIX='\".so\"'
    -DUNITY_BACKEND_DIR='\"\"')

# concatenate (join) strings to make single argument
FOREACH(ud ${unity_DEFINE})
  set(unity_DEFINES "${unity_DEFINES} ${ud}")
ENDFOREACH(ud ${unity_DEFINE})

SET_SOURCE_FILES_PROPERTIES(
    cam_iface_unity.c
    PROPERTIES COMPILE_FLAGS ${unity_DEFINES})

ADD_LIBRARY(camiface_unity SHARED ${unity_SRCS})
TARGET_LINK_LIBRARIES(camiface_unity dl)

INSTALL(TARGETS camiface_unity
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

# dc1394 backend ------------------

IF(DC1394_FOUND)

  set(dc1394_SRCS ${common_SRCS}
      cam_iface_dc1394.c
      )

  ADD_LIBRARY(camiface_dc1394 SHARED ${dc1394_SRCS})
  TARGET_LINK_LIBRARIES(camiface_dc1394 ${DC1394_LIBRARIES})

  INSTALL(TARGETS camiface_dc1394
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

ENDIF(DC1394_FOUND)
