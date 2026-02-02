scada_module(scada_common_opc)

target_include_directories(scada_common_opc PUBLIC "..")

target_link_libraries(scada_common_opc PUBLIC scada_core)

target_link_libraries(scada_common_opc PRIVATE scada_model)

find_package(OPC REQUIRED)
target_link_libraries(scada_common_opc PRIVATE OPC::opc)

find_package(OpcClient REQUIRED)
# Apply `PUBLIC` scope since `opc_client::DataValue` is used in the public API.
target_link_libraries(scada_common_opc PUBLIC opc_client)

# UTs.

target_link_libraries(scada_common_opc_unittests PRIVATE
  OPC::opc
  # Uses `MakeNestedNodeId()` directly.
  scada_model
)