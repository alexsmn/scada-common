scada_module(scada_common_vidicon)

target_link_libraries(scada_common_vidicon PUBLIC scada_core)

target_link_libraries(scada_common_vidicon PRIVATE scada_common_opc)

target_include_directories(scada_common_vidicon PUBLIC "..")

set(CMAKE_SKIP_LINTING ON)
find_package(MIDL REQUIRED)
set(CMAKE_SKIP_LINTING OFF)
add_idl(teleclient_idl "TeleClient.idl" INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scada_common_vidicon PUBLIC teleclient_idl)
target_include_directories(scada_common_vidicon PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# UTs.

target_link_libraries(scada_common_vidicon_unittests PRIVATE scada_common_opc)

# Subdirectories.

add_subdirectory(services)