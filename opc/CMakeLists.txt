scada_module(scada_common_opc)

target_include_directories(scada_common_opc PUBLIC "..")

target_link_libraries(scada_common_opc PUBLIC scada_core)

target_link_libraries(scada_common_opc PRIVATE scada_model)

set(CMAKE_SKIP_LINTING ON)
find_package(OPC REQUIRED)
find_package(OpcClient REQUIRED)
set(CMAKE_SKIP_LINTING OFF)

target_link_libraries(scada_common_opc PRIVATE OPC::opc)
# Apply `PUBLIC` scope since `opc_client::DataValue` is used in the public API.
target_link_libraries(scada_common_opc PUBLIC opc_client)

# UTs.

target_link_libraries(scada_common_opc_unittests PRIVATE
  OPC::opc
  # Uses `MakeNestedNodeId()` directly.
  scada_model
)