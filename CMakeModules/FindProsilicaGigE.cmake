FIND_PATH(PROSILICA_GIGE_INCLUDE_PATH PvApi.h
  /usr/local/include
  /usr/include
)

FIND_LIBRARY(PROSILICA_GIGE_LIBRARY PvAPI /usr/local/lib /usr/lib)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PROSILICA_GIGE DEFAULT_MSG 
                                                 PROSILICA_GIGE_INCLUDE_PATH
                                                 PROSILICA_GIGE_LIBRARY)
SET(PROSILICA_GIGE_LIBRARIES ${PROSILICA_GIGE_LIBRARY})