scada_module(node_service)

target_include_directories(node_service PUBLIC "..")

target_link_libraries(node_service PUBLIC scada_common)

target_link_libraries(node_service_unittests PUBLIC
  address_space
  node_service
)

# Subdirectories

add_subdirectory(v1)
add_subdirectory(v2)
add_subdirectory(v3)
