scada_module(address_space)

target_precompile_headers(address_space REUSE_FROM scada_base)

target_include_directories(address_space PUBLIC "..")

target_link_libraries(address_space PUBLIC
  scada_core
  transport
)

find_package(pugixml REQUIRED)

target_link_libraries(address_space PRIVATE
  scada_common
  scada_model
  pugixml::pugixml
)

# Subdirectories.

add_subdirectory(test)
