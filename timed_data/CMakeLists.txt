scada_module(timed_data)

# No PCH in the modules pilot (see core docs: scan-deps cannot read it).
if(NOT SCADA_CXX_MODULES)
  target_precompile_headers(timed_data REUSE_FROM scada_base)
endif()

target_include_directories(timed_data PUBLIC "..")

target_link_libraries(timed_data PRIVATE
  node_service
  scada_common
  transport
)

# UTs.

target_link_libraries(timed_data_unittests PRIVATE
  address_space
  express
)

# Modules-pilot consumer: base_timed_data.cpp imports scada.core (see
# SCADA_USE_CORE_MODULE there).
scada_module_import_pilot(timed_data
  IMPORTS scada_core_module
  DEFINES SCADA_USE_CORE_MODULE)

scada_add_module_facade(timed_data FILES scada_timed_data.cppm
  IMPORTS scada_common_module scada_common_events_module)
if(SCADA_CXX_MODULES)
  add_subdirectory(module_test)
endif()
