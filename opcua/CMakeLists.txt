find_package(OPCUAPP REQUIRED)

scada_module(scada_core_opcua)
scada_module_sources(scada_core_opcua PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/binary"
  RECURSE)

target_precompile_headers(scada_core_opcua REUSE_FROM scada_base)

target_include_directories(scada_core_opcua PUBLIC "..")

set(OPCUA_LEGACY_CLIENT_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_monitored_item.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_monitored_item.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_services_factory.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_session.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_session.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_subscription.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_subscription.h"
)

get_target_property(OPCUA_MODULE_SOURCES scada_core_opcua SOURCES)
list(REMOVE_ITEM OPCUA_MODULE_SOURCES ${OPCUA_LEGACY_CLIENT_SOURCES})
set_property(TARGET scada_core_opcua PROPERTY SOURCES ${OPCUA_MODULE_SOURCES})

target_include_directories(scada_core_opcua PUBLIC
  $<TARGET_PROPERTY:OPCUAPP,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(scada_core_opcua PUBLIC
  $<TARGET_PROPERTY:OPCUAPP,INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(scada_core_opcua PUBLIC
  $<TARGET_PROPERTY:OPCUAPP,INTERFACE_COMPILE_OPTIONS>
)

target_link_libraries(scada_core_opcua PUBLIC
  scada_core
  transport
)

add_library(scada_core_opcua_client
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_monitored_item.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_monitored_item.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_services_factory.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_session.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_session.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_subscription.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/opcua_subscription.h"
)

target_precompile_headers(scada_core_opcua_client REUSE_FROM scada_base)
target_include_directories(scada_core_opcua_client PUBLIC "..")
target_link_libraries(scada_core_opcua_client PUBLIC
  scada_core_opcua
  OPCUAPP
)

if(TARGET scada_core_opcua_unittests)
  target_link_libraries(scada_core_opcua_unittests PRIVATE OPCUAPP)
endif()
