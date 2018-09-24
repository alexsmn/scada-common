find_package(OPCUAPP REQUIRED)

file(GLOB sources "*.cpp" "*.h")

add_library(scada_core_opcua ${sources})

target_link_libraries(scada_core_opcua scada_core OPCUAPP)
