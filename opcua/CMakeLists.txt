scada_module(scada_core_opcua)
scada_module_sources(scada_core_opcua PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/binary"
  RECURSE)

target_precompile_headers(scada_core_opcua REUSE_FROM scada_base)

target_include_directories(scada_core_opcua PUBLIC "..")

# The legacy opcuapp-backed conversion helpers are still on disk pending a
# tree-wide cleanup; exclude them from the compilation until the user
# authorizes their deletion. The Qt-client-facing wrappers
# (opcua_session/subscription/monitored_item/services_factory) ship with
# scada_core_opcua now that they're built on top of the native binary
# client under common/opcua/binary/client/.
set(OPCUA_DEAD_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_conversion.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_conversion.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_conversion_unittest.cpp"
)

get_target_property(OPCUA_MODULE_SOURCES scada_core_opcua SOURCES)
list(REMOVE_ITEM OPCUA_MODULE_SOURCES ${OPCUA_DEAD_SOURCES})
set_property(TARGET scada_core_opcua PROPERTY SOURCES ${OPCUA_MODULE_SOURCES})

if(TARGET scada_core_opcua_unittests)
  get_target_property(OPCUA_UNITTESTS_SOURCES scada_core_opcua_unittests SOURCES)
  list(REMOVE_ITEM OPCUA_UNITTESTS_SOURCES ${OPCUA_DEAD_SOURCES})
  set_property(TARGET scada_core_opcua_unittests PROPERTY SOURCES
               ${OPCUA_UNITTESTS_SOURCES})
endif()

target_link_libraries(scada_core_opcua PUBLIC
  scada_core
  transport
)

# scada_core_opcua_client is retained as an alias for backwards compatibility
# with downstream CMakeLists that still target it directly. It now links the
# same sources as scada_core_opcua (which already ships the session / sub /
# monitored-item / factory translation units) and no longer pulls in the
# external opcuapp dependency.
add_library(scada_core_opcua_client INTERFACE)
target_link_libraries(scada_core_opcua_client INTERFACE scada_core_opcua)
