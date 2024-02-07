scada_module(timed_data)

target_include_directories(timed_data PUBLIC "..")

target_link_libraries(timed_data PRIVATE
  node_service
  scada_common
)

# UTs.

target_link_libraries(timed_data_unittests PRIVATE
  address_space
  express
)
