IF(WIN32)
  SET(PROSILICA_TEST_INCLUDE_PATHS
     "C:\\Program Files\\Prosilica\\GigESDK\\inc-pc"
     )
  SET(PROSILICA_TEST_LIB_PATHS
     "C:\\Program Files\\Prosilica\\GigESDK\\lib-pc"
     )
ELSE(WIN32)
  SET(PROSILICA_TEST_INCLUDE_PATHS
     /usr/local/include
     /usr/include
     )
  SET(PROSILICA_TEST_LIB_PATHS
      /usr/local/lib 
      /usr/lib
     )
ENDIF(WIN32)

FIND_PATH(PROSILICA_GIGE_INCLUDE_PATH PvApi.h
  ${PROSILICA_TEST_INCLUDE_PATHS}
)

FIND_LIBRARY(PROSILICA_GIGE_LIBRARY PvAPI ${PROSILICA_TEST_LIB_PATHS})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PROSILICA_GIGE DEFAULT_MSG 
                                                 PROSILICA_GIGE_INCLUDE_PATH
                                                 PROSILICA_GIGE_LIBRARY)
SET(PROSILICA_GIGE_LIBRARIES ${PROSILICA_GIGE_LIBRARY})
SET(PROSILICA_GIGE_INCLUDE_DIRS ${PROSILICA_GIGE_INCLUDE_PATH})
