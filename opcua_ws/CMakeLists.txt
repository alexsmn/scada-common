scada_module(scada_common_opcua_ws)

target_precompile_headers(scada_common_opcua_ws REUSE_FROM scada_base)

target_include_directories(scada_common_opcua_ws PUBLIC "..")

target_link_libraries(scada_common_opcua_ws PUBLIC
  scada_core
  transport
)
