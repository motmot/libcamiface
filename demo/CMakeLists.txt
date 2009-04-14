set(simple_SRCS
    simple.c
    )

ADD_EXECUTABLE(simple ${simple_SRCS})
TARGET_LINK_LIBRARIES(simple camiface_unity)

INSTALL(TARGETS simple
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
