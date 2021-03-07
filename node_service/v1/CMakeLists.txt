scada_module(node_service_v1)

target_include_directories(node_service_v1 PUBLIC "../..")
target_link_libraries(node_service_v1 PUBLIC
  address_space
  node_service
)
