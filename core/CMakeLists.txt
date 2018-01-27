file(GLOB scada_core_sources "*.cpp" "*.h")
file(GLOB scada_core_unittest_sources "*_unittest.*")
list(REMOVE_ITEM scada_core_sources ${scada_core_unittest_sources})

add_library(scada_core ${scada_core_sources})
target_link_libraries(scada_core scada_base)

target_compile_options(scada_core PUBLIC "/std:c++latest")

# Conformance mode
target_compile_options(scada_core PRIVATE "/Za" "/permissive-")

add_executable(scada_core_unittests ${scada_core_unittest_sources})
target_link_libraries(scada_core_unittests scada_core gmock_main)
add_test(NAME scada_core_unittests COMMAND scada_core_unittests)
