find_package(OpenSSL REQUIRED)

scada_module(scada_core_opcua)
scada_module_sources(scada_core_opcua PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/binary"
  RECURSE)

target_precompile_headers(scada_core_opcua REUSE_FROM scada_base)

target_include_directories(scada_core_opcua PUBLIC "..")

target_link_libraries(scada_core_opcua PUBLIC
  scada_core
  transport
)

# The Basic256Sha256 crypto helper under binary/client/ is the only
# consumer of OpenSSL in this module; keep the link private.
target_link_libraries(scada_core_opcua PRIVATE
  OpenSSL::SSL
  OpenSSL::Crypto
)

if(TARGET scada_core_opcua_unittests)
  # opcua_binary_crypto_unittest.cpp generates its RSA test fixtures via
  # OpenSSL directly.
  target_link_libraries(scada_core_opcua_unittests PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
  )
endif()

# scada_core_opcua_client is retained as an alias for backwards compatibility
# with downstream CMakeLists that still target it directly. It now forwards
# to scada_core_opcua (which already ships the outbound-client session /
# subscription / monitored-item / factory translation units) and no longer
# pulls in the external opcuapp dependency.
add_library(scada_core_opcua_client INTERFACE)
target_link_libraries(scada_core_opcua_client INTERFACE scada_core_opcua)
