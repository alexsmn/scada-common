scada_module(address_space)

# No PCH in the modules pilot (see core docs: scan-deps cannot read it).
if(NOT SCADA_CXX_MODULES)
  target_precompile_headers(address_space REUSE_FROM scada_base)
endif()

target_include_directories(address_space PUBLIC "..")

target_link_libraries(address_space PUBLIC
  scada_core
  transport
  # Node-id / namespace vocabulary (model/*.h) is used by address_space's public
  # headers (incl. the test/ helpers other modules' tests reuse); PUBLIC so
  # consumers resolve "model/..." through this link.
  scada_model
)

find_package(pugixml REQUIRED)

target_link_libraries(address_space PRIVATE
  scada_common
  pugixml::pugixml
)

# Modules-pilot consumer: local_method_service.cpp imports scada.core (see
# SCADA_USE_CORE_MODULE there).
scada_module_import_pilot(address_space
  IMPORTS scada_core_module
  DEFINES SCADA_USE_CORE_MODULE)

scada_add_module_facade(address_space FILES scada_address_space.cppm
  IMPORTS scada_core_module)
if(SCADA_CXX_MODULES)
  add_subdirectory(module_test)
endif()

# Subdirectories.

add_subdirectory(test)
