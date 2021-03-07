scada_module(node_service)

target_include_directories(node_service PUBLIC "..")

target_link_libraries(node_service PUBLIC scada_common)

target_link_libraries(node_service_unittests PUBLIC
  address_space
  node_service
)

# Subdirectories

add_subdirectory(address_space)
add_subdirectory(remote)
add_subdirectory(v3)