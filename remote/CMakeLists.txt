# ProtoBuf
# ADD_SUBDIRECTORY("${deps}/protobuf" "protobuf-build")
find_package(Protobuf REQUIRED)

file(GLOB sources "*.cpp" "*.h" "*.proto")

# list(APPEND remote_sources "scada.pb.cc" "scada.pb.h")

file(GLOB unittests "*_unittest.*")

list(REMOVE_ITEM sources ${unittests})

protobuf_generate_cpp(scada_proto_cpp scada_proto_h scada.proto)
protobuf_generate_python(scada_proto_py scada.proto)
#set(CMAKE_INCLUDE_CURRENT_DIR ON)

list(APPEND sources ${scada_proto_cpp} ${scada_proto_h})

add_library(scada_core_remote ${sources})

target_include_directories(scada_core_remote PUBLIC ${Protobuf_INCLUDE_DIRS})

# For proto generated files
target_include_directories(scada_core_remote PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(scada_core_remote
  ${Protobuf_LIBRARIES}
  net
  scada_core
)

add_executable(scada_core_remote_unittests ${unittests})
add_test(NAME scada_core_remote_unittests COMMAND scada_core_remote_unittests)
target_link_libraries(scada_core_remote_unittests	scada_core_remote	gmock_main)
