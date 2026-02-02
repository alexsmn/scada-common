find_package(ScadaCore REQUIRED)
find_package(Express REQUIRED)

add_subdirectory(common)
add_subdirectory(events)
add_subdirectory(address_space)
add_subdirectory(opcua)
add_subdirectory(timed_data)
add_subdirectory(node_service)

if(WIN32)
  add_subdirectory(opc)
  add_subdirectory(vidicon)
endif()
