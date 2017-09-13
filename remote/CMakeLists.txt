file(GLOB remote_sources
  "*.cpp"
  "*.h"
  "*.proto"
)

list(APPEND remote_sources
  "scada.pb.cc"
  "scada.pb.h"
)

file(GLOB remote_ut_sources
  "*_unittest.*"
)

list(REMOVE_ITEM remote_sources ${remote_ut_sources})

add_custom_command(
  OUTPUT
  "${CMAKE_CURRENT_SOURCE_DIR}/scada.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/scada.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/scada_pb2.py"
  COMMAND protoc --cpp_out=. --python_out=. scada.proto
  MAIN_DEPENDENCY scada.proto
  DEPENDS protoc.exe
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS scada.proto
)

add_library(remote ${remote_sources})

target_link_libraries(remote
  net
  protobuf
  scada_core
)

add_executable(remote_unittests ${remote_ut_sources})
add_test(NAME remote_unittests COMMAND remote_unittests)
target_link_libraries(remote_unittests
	remote
	gmock_main
)
