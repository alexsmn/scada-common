# Boundary adapter between the SCADA `core` types (scada::) and the extracted
# opcuapp protocol types (opcua::). Depends on both core and opcuapp; the two
# type universes are namespaced apart so they coexist in one translation unit
# without ODR clashes.
#
scada_module(scada_opcua_bridge)

find_package(opcuapp REQUIRED)

# The bridge includes both opcuapp's `opcua/...` headers and (for its own
# `opcua_bridge/...` headers) the common/ tree. Put opcuapp first so
# `#include "opcua/client/client_session.h"` resolves there.
target_include_directories(scada_opcua_bridge BEFORE PUBLIC
  $<TARGET_PROPERTY:opcuapp,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(scada_opcua_bridge PUBLIC "..")

target_link_libraries(scada_opcua_bridge PUBLIC
  scada_core
  opcuapp::opcuapp
)

# Modules-pilot consumer: remote_history_service.cpp imports scada.core (see
# SCADA_USE_CORE_MODULE there).
scada_module_import_pilot(scada_opcua_bridge
  IMPORTS scada_core_module
  DEFINES SCADA_USE_CORE_MODULE)

scada_add_module_facade(scada_opcua_bridge FILES scada_opcua_bridge.cppm
  IMPORTS scada_core_module)
if(SCADA_CXX_MODULES)
  add_subdirectory(module_test)
endif()
