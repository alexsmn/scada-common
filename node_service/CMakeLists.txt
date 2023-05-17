scada_module(node_service)
scada_module_sources(node_service "static")

target_include_directories(node_service PUBLIC "..")

target_link_libraries(node_service PUBLIC
	scada_common
)

# Implementations.

add_subdirectory(v1)
add_subdirectory(v2)
add_subdirectory(v3)
add_subdirectory(proxy)

# Link implementations to common UTs.

target_link_libraries(node_service_unittests PUBLIC
  node_service_v1
  node_service_v2
  node_service_v3
  node_service_proxy
)
