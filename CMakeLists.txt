# OPC-UA
set(OPENSSL_ROOT_DIR "${deps}/openssl")
set(OPCUA_ROOT_DIR "${deps}/UA-AnsiC")
set(OPCUA_DIR "${deps}/opcuapp")
set(OPCUAPP_DIR "${deps}/opcuapp")

add_subdirectory("${deps}/express" express)

add_subdirectory(common)
add_subdirectory(address_space)
add_subdirectory(opcua)
add_subdirectory(timed_data)
add_subdirectory(node_service)

if(WIN32)
  add_subdirectory(vidicon)
endif()
