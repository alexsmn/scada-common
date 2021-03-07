scada_module(node_service_v2)
target_include_directories(node_service_v2 PUBLIC "../..")
target_link_libraries(node_service_v2 PUBLIC
  node_service
)

target_link_libraries(node_service_v2_unittests PUBLIC
  address_space
 )
