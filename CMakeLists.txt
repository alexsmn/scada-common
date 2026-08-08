cmake_minimum_required(VERSION 3.27)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
# The shared build-support kit (ADR 0011): at this product's root in an export,
# at the tree root in the monorepo. Before project(), so the machine config it
# reads can still set the vcpkg variables the toolchain consumes there.
set(_scada_dir "${CMAKE_CURRENT_SOURCE_DIR}")
while(NOT EXISTS "${_scada_dir}/build-support/ScadaProducts.cmake")
  get_filename_component(_scada_parent "${_scada_dir}" DIRECTORY)
  if(_scada_parent STREQUAL _scada_dir)
    message(FATAL_ERROR
      "build-support/ not found above ${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
  set(_scada_dir "${_scada_parent}")
endwhile()
include("${_scada_dir}/build-support/ScadaProducts.cmake")
include("${_scada_dir}/build-support/ScadaProductBase.cmake")
unset(_scada_dir)
unset(_scada_parent)

scada_product_prologue()

project(scada-common VERSION 2.6.0 LANGUAGES C CXX)

scada_product_base()

# The products this one consumes. `opcuapp` backs the scada_core_opcua shims
# below and opcua_bridge; `express` is the expression evaluator; `core` is the
# base. On Windows the opc/ and vidicon/ subtrees additionally reach genuine
# out-of-tree SDKs (OPC Foundation, midl) — those are not products, so they come
# from SCADA_EXTRA_MODULE_PATH in the machine config rather than from here.
scada_find_products(core express opcuapp)

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

add_subdirectory(model)

# Targets that include "model/..." headers link `scada_model` (or a lib that
# PUBLIC-links it, e.g. scada_common / server_base): scada_model PUBLIC-exports
# both the common root and the generated headers dir. No global include hack —
# an explicit link keeps the dependency graph honest so a subset of the tree
# (e.g. a standalone server tier) can build without this top-level CMake.

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

# common/vds_runtime_api.h is the plugin ABI between designer, which implements
# the VDS runtime, and the client, which loads it. Neither owns it, which is why
# it lives here. This carries just its include root, so a consumer needing only
# the contract does not link the whole of scada_common for it.
add_library(scada_vds_runtime_api INTERFACE)
target_include_directories(scada_vds_runtime_api INTERFACE "${CMAKE_CURRENT_LIST_DIR}")

add_subdirectory(timed_data)
add_subdirectory(node_service)

# The shared E2E harness. Declares INTERFACE targets only, so adding it here
# costs nothing to the framework, the tiers and the client, which all build this
# repository and none of which link an E2E harness.
add_subdirectory(test/e2e)

option(BUILD_OPC "Build Classic OPC modules (Windows only)" ON)
option(BUILD_VIDICON "Build Vidicon modules (Windows only)" ON)

if(WIN32 AND BUILD_OPC)
  add_subdirectory(opc)
endif()

if(WIN32 AND BUILD_VIDICON)
  add_subdirectory(vidicon)
endif()
