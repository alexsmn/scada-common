scada_module(scada_common_opc)

target_include_directories(scada_common_opc PUBLIC "..")
target_link_libraries(scada_common_opc PUBLIC scada_core)

find_package(OPC REQUIRED)
target_link_libraries(scada_common_opc PUBLIC OPC::opc)

# TODO: Update to `find_package`.
# TODO: Split `opc_client` to `opc_client::core` and reference it here.
add_subdirectory("${deps}/opc_client" "opc_client-build-2")
target_link_libraries(scada_common_opc PRIVATE opc_client)