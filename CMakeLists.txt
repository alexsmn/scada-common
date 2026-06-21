cmake_minimum_required(VERSION 3.27)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
project(scada-common VERSION 2.6.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
  add_definitions(
    -DWINVER=0x0603
    -D_WIN32_WINNT=0x0603
    -D_MSVC_STL_HARDENING
  )
endif()

# IDE folder for targets (parent project may override).
if(NOT DEFINED scada_common_folder)
  set(scada_common_folder "Common")
endif()

enable_testing()

# Skip linting for third-party dependencies.
set(CMAKE_SKIP_LINTING ON)
find_package(ScadaCore REQUIRED)
find_package(Express REQUIRED)
set(CMAKE_SKIP_LINTING OFF)

add_subdirectory(common)
add_subdirectory(events)
add_subdirectory(address_space)
add_subdirectory(opcua_bridge)

# The in-tree OPC UA native stack has been extracted to the standalone opcuapp
# repo (third_party/opcuapp). Keep the old target names as INTERFACE shims so
# existing consumers continue to link through the same CMake targets.
find_package(opcuapp REQUIRED)

add_library(scada_core_opcua INTERFACE)
target_include_directories(scada_core_opcua BEFORE INTERFACE
  $<TARGET_PROPERTY:opcuapp,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(scada_core_opcua INTERFACE
  opcuapp::opcuapp
  scada_opcua_bridge
)

add_library(scada_core_opcua_client INTERFACE)
target_link_libraries(scada_core_opcua_client INTERFACE scada_core_opcua)

add_library(scada_common_opcua_ws INTERFACE)
target_link_libraries(scada_common_opcua_ws INTERFACE scada_core_opcua)

add_subdirectory(timed_data)
add_subdirectory(node_service)

option(BUILD_OPC "Build Classic OPC modules (Windows only)" ON)
option(BUILD_VIDICON "Build Vidicon modules (Windows only)" ON)

if(WIN32 AND BUILD_OPC)
  add_subdirectory(opc)
endif()

if(WIN32 AND BUILD_VIDICON)
  add_subdirectory(vidicon)
endif()
