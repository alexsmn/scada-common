scada_module(scada_common_vidicon)

target_link_libraries(scada_common_vidicon PUBLIC scada_core)

target_include_directories(scada_common_vidicon PUBLIC "..")

find_package(MIDL REQUIRED)
add_idl(teleclient_idl "TeleClient.idl" INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scada_common_vidicon PUBLIC teleclient_idl)
target_include_directories(scada_common_vidicon PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Subdirectories.

add_subdirectory(services)