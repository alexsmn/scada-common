scada_module(timed_data)

target_include_directories(timed_data PUBLIC "..")

target_link_libraries(timed_data PUBLIC
  node_service
  scada_common
)