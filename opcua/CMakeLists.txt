# The in-tree OPC UA native stack has been extracted to the standalone opcuapp
# repo (third_party/opcuapp). `scada_core_opcua` and `scada_common_opcua_ws` are
# now thin shims that expose opcuapp plus the scada<->opcua boundary adapter
# (scada_opcua_bridge), so existing consumers keep linking the same target names.
find_package(opcuapp REQUIRED)

add_library(scada_core_opcua INTERFACE)
# opcuapp's include directory must precede everything so `#include "opcua/..."`
# resolves to opcuapp (this directory no longer ships those headers).
target_include_directories(scada_core_opcua BEFORE INTERFACE
  $<TARGET_PROPERTY:opcuapp,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(scada_core_opcua INTERFACE
  opcuapp::opcuapp
  scada_opcua_bridge
)

# Backwards-compatible aliases for downstream targets that linked the old names.
add_library(scada_core_opcua_client INTERFACE)
target_link_libraries(scada_core_opcua_client INTERFACE scada_core_opcua)

add_library(scada_common_opcua_ws INTERFACE)
target_link_libraries(scada_common_opcua_ws INTERFACE scada_core_opcua)
