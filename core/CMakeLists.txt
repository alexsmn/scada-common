find_package(OPCUAPP REQUIRED)

FILE(GLOB scada_core_sources
  "*.cpp"
  "*.h")

FILE(GLOB scada_core_unittest_sources
  "*_unittest.*")
LIST(REMOVE_ITEM scada_core_sources ${scada_core_unittest_sources})

ADD_LIBRARY(scada_core STATIC ${scada_core_sources})
TARGET_LINK_LIBRARIES(scada_core
  scada_base
  OPCUAPP
)

ADD_EXECUTABLE(scada_core_unittests ${scada_core_unittest_sources})
TARGET_LINK_LIBRARIES(scada_core_unittests
  scada_core
  gmock_main
)
ADD_TEST(NAME scada_core_unittests COMMAND scada_core_unittests)
