scada_module(scada_common)

# No PCH in the modules pilot: clang-scan-deps cannot read the AppleClang
# PCH and silently emits an empty dependency scan (see core docs).
if(NOT SCADA_CXX_MODULES)
  target_precompile_headers(scada_common REUSE_FROM scada_base)
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/project.h.src"
  "project.h"
)

# Add project.h to includes.
target_include_directories(scada_common PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(scada_common PUBLIC "..")

target_link_libraries(scada_common PUBLIC
  scada_core
  scada_metrics
  transport
  # Node-id / namespace vocabulary (model/*.h, e.g. NodeIdFromScadaString) is part
  # of scada_common's public surface; PUBLIC so consumers resolve "model/..."
  # through this link instead of a global include hack.
  scada_model
)

target_link_libraries(scada_common PRIVATE
  express
)

# Modules-pilot consumer: node_state_util.cpp imports scada.core (see
# SCADA_USE_CORE_MODULE there).
scada_module_import_pilot(scada_common
  IMPORTS scada_core_module
  DEFINES SCADA_USE_CORE_MODULE)

scada_add_module_facade(scada_common FILES scada_common.cppm
  IMPORTS scada_core_module scada_metrics_module)
if(SCADA_CXX_MODULES)
  add_subdirectory(module_test)
endif()

# UT

# scada_module() globs this directory but not its subdirectories, so the
# ScopedTempDir test has to be named. It lives beside the header it tests, in
# test/, because that is the path every consuming product includes it by
# (`common/test/scoped_temp_dir.h`) -- and a test file no target compiles looks
# exactly like a passing one.
scada_module_unittests(scada_common
  test/scoped_temp_dir_unittest.cpp
)

target_link_libraries(scada_common_unittests PUBLIC
  address_space
)
