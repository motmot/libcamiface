set(common_SRCS
    cam_iface_common.c
    )

set(unity_SRCS ${common_SRCS}
    cam_iface_unity.c
    )

SET_SOURCE_FILES_PROPERTIES(
    cam_iface_unity.c
    PROPERTIES COMPILE_FLAGS
    "-DUNITY_BACKEND_PREFIX='\"lib\"' -DUNITY_BACKEND_SUFFIX='\".so\"' -DUNITY_BACKEND_DIR='\"\"'")

ADD_LIBRARY(camiface_unity SHARED ${unity_SRCS})
TARGET_LINK_LIBRARIES(camiface_unity dl)

INSTALL(TARGETS camiface_unity
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
