# ProtoBuf
# ADD_SUBDIRECTORY("${deps}/protobuf" "protobuf-build")
find_package(Protobuf REQUIRED)

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

protobuf_generate_cpp(scada_proto_cpp scada_proto_h scada.proto)
protobuf_generate_python(scada_proto_py scada.proto)
#set(CMAKE_INCLUDE_CURRENT_DIR ON)

list(APPEND remote_sources ${scada_proto_cpp} ${scada_proto_h})

add_library(remote ${remote_sources})

target_include_directories(remote PUBLIC ${Protobuf_INCLUDE_DIRS})

# For proto generated files
target_include_directories(remote PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(remote
  ${Protobuf_LIBRARIES}
  net
  scada_core
)

add_executable(remote_unittests ${remote_ut_sources})
add_test(NAME remote_unittests COMMAND remote_unittests)
target_link_libraries(remote_unittests
	remote
	gmock_main
)
