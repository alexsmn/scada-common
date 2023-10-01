scada_module(node_service_v1)

target_include_directories(node_service_v1 PUBLIC "../..")

target_link_libraries(node_service_v1 PUBLIC
  node_service
)

target_link_libraries(node_service_v1 PRIVATE
  address_space
)
