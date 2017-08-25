FILE(GLOB remote_sources
  "*.cpp"
  "*.h"
  "*.proto")

LIST(APPEND remote_sources
  "scada.pb.cc"
  "scada.pb.h")

FILE(GLOB remote_ut_sources
  "*_unittest.*")

LIST(REMOVE_ITEM remote_sources ${remote_ut_sources})

ADD_CUSTOM_COMMAND(
  OUTPUT
  "${CMAKE_CURRENT_SOURCE_DIR}/scada.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/scada.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/scada_pb2.py"
  COMMAND protoc --cpp_out=. --python_out=. scada.proto
  MAIN_DEPENDENCY scada.proto
  DEPENDS protoc.exe
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS scada.proto)

ADD_LIBRARY(remote STATIC ${remote_sources})

TARGET_LINK_LIBRARIES(remote
  net
  protobuf
  scada_base
  scada_common)

ADD_EXECUTABLE(remote_unittests ${remote_ut_sources})
ADD_TEST(NAME remote_unittests COMMAND remote_unittests)
TARGET_LINK_LIBRARIES(remote_unittests remote gmock_main)
