CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/project.h.src"
  "${CMAKE_CURRENT_SOURCE_DIR}/project.h")

FILE(GLOB scada_common_sources
  "*.cpp"
  "*.h"
  "timed_data/*.cpp"
  "timed_data/*.h"
  "tree/*.cpp"
  "tree/*.h")

FILE(GLOB scada_common_ut_sources
  "*_unittest.*")

LIST(REMOVE_ITEM scada_common_sources ${scada_common_ut_sources})

ADD_LIBRARY(scada_common STATIC ${scada_common_sources})
TARGET_LINK_LIBRARIES(scada_common
  base
  express
  scada_base
  scada_core)

ADD_EXECUTABLE(scada_common_unittests ${scada_common_ut_sources})
TARGET_LINK_LIBRARIES(scada_common_unittests
  scada_common
  gmock_main)
ADD_TEST(NAME scada_common_unittests COMMAND scada_common_unittests)
