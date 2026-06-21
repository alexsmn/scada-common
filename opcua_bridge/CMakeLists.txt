# Boundary adapter between the SCADA `core` types (scada::) and the extracted
# opcuapp types (opcua::scada::). Depends on both core and opcuapp; the two type
# universes are namespaced apart (opcuapp is fully wrapped in `namespace opcua`)
# so they coexist in one translation unit without ODR clashes.
#
# NOTE: this module requires the `opcuapp` package to be discoverable
# (find_package(opcuapp) via CMAKE_MODULE_PATH). It is intentionally NOT yet
# wired into the parent build; it is added once the build points at opcuapp.
scada_module(scada_opcua_bridge)

find_package(opcuapp REQUIRED)

# The bridge includes both opcuapp's `opcua/...` headers and (for its own
# `opcua_bridge/...` headers) the common/ tree, which ALSO contains an `opcua/`
# directory (the in-tree native stack, until it is removed in the cutover).
# opcuapp's include directory MUST take precedence so that e.g.
# `#include "opcua/client_session.h"` resolves to opcuapp, not common/opcua.
target_include_directories(scada_opcua_bridge BEFORE PUBLIC
  $<TARGET_PROPERTY:opcuapp,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(scada_opcua_bridge PUBLIC "..")

target_link_libraries(scada_opcua_bridge PUBLIC
  scada_core
  opcuapp::opcuapp
)
