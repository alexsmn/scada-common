FILE(GLOB sources
  "*.cpp"
  "*.h")

FILE(GLOB sources_win
  "win/*.cpp"
  "win/*.h"
)

IF(WIN32)
  LIST(APPEND sources ${sources_win})
ENDIF()

ADD_LIBRARY(scada_base STATIC ${sources})

TARGET_LINK_LIBRARIES(scada_base base)