scada_module(node_service_proxy)

target_precompile_headers(node_service_proxy REUSE_FROM scada_base)

target_link_libraries(node_service_proxy PUBLIC
  node_service
)