cmake_minimum_required(VERSION 3.27)
project(scada-common VERSION 2.5.7 LANGUAGES C CXX)

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
add_subdirectory(opcua)
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
