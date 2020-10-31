scada_module(remote_node_service)
target_include_directories(remote_node_service PUBLIC "../..")
target_link_libraries(remote_node_service PUBLIC
  node_service
)

target_link_libraries(remote_node_service_unittests PUBLIC
  address_space
 )
