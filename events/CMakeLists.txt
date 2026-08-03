scada_module(scada_common_events)

# No PCH in the modules pilot (see core docs: scan-deps cannot read it).
if(NOT SCADA_CXX_MODULES)
  target_precompile_headers(scada_common_events REUSE_FROM scada_base)
endif()

target_include_directories(scada_common_events PUBLIC "..")

target_link_libraries(scada_common_events PUBLIC
  scada_common
  transport
)

# Modules-pilot consumer: event_storage.cpp imports scada.core (see
# SCADA_USE_CORE_MODULE there).
scada_module_import_pilot(scada_common_events
  IMPORTS scada_core_module
  DEFINES SCADA_USE_CORE_MODULE)

scada_add_module_facade(scada_common_events FILES scada_events.cppm
  IMPORTS scada_common_module)
if(SCADA_CXX_MODULES)
  add_subdirectory(module_test)
endif()
