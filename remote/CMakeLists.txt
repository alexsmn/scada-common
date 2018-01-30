# ProtoBuf
find_package(Protobuf REQUIRED)

file(GLOB sources "*.cpp" "*.h" "*.proto")
file(GLOB unittests "*_unittest.*")

list(REMOVE_ITEM sources ${unittests})

protobuf_generate_cpp(scada_proto_cpp scada_proto_h scada.proto)
protobuf_generate_python(scada_proto_py scada.proto)

list(APPEND sources ${scada_proto_cpp} ${scada_proto_h})

add_library(scada_core_remote ${sources})

# For proto generated files
target_include_directories(scada_core_remote PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(scada_core_remote PUBLIC
  protobuf::libprotobuf
  net
  scada_core
)

add_executable(scada_core_remote_unittests ${unittests})
add_test(NAME scada_core_remote_unittests COMMAND scada_core_remote_unittests)
target_link_libraries(scada_core_remote_unittests PUBLIC
  scada_core_remote
  gmock_main
)
