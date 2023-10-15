scada_module(address_space)

target_include_directories(address_space PUBLIC "..")

target_link_libraries(address_space PUBLIC
  scada_core
)

target_link_libraries(address_space PRIVATE
  scada_common
  scada_model
)

# Subdirectories.

add_subdirectory(test)