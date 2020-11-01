scada_module(address_space_node_service)

target_include_directories(address_space_node_service PUBLIC "../..")
target_link_libraries(address_space_node_service PUBLIC
  address_space
  node_service
)

target_link_libraries(address_space_node_service_unittests PUBLIC
  address_space_node_service
)