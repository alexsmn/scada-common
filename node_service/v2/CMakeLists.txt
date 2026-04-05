scada_module(node_service_v2)

target_precompile_headers(node_service_v2 REUSE_FROM scada_base)

target_include_directories(node_service_v2 PUBLIC "../..")

target_link_libraries(node_service_v2 PUBLIC
  node_service
)
