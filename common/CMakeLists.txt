scada_module(scada_common)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/project.h.src"
  "project.h"
)

# Add project.h to includes.
target_include_directories(scada_common PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(scada_common PUBLIC "..")

target_link_libraries(scada_common PUBLIC
  base
  express
  scada_base
  scada_core
)

# UT

target_link_libraries(scada_common_unittests PUBLIC
  address_space
)
