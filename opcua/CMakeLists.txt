find_package(OPCUAPP REQUIRED)

scada_module(scada_core_opcua)

target_precompile_headers(scada_core_opcua REUSE_FROM scada_base)

target_include_directories(scada_core_opcua PUBLIC "..")

target_link_libraries(scada_core_opcua PUBLIC
  scada_core
  scada_common_opcua_ws
  OPCUAPP
)
