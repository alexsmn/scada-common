file(GLOB sources
  "*.cpp"
  "*.h"
  "../timed_data/*.cpp"
  "../timed_data/*.h"
)

file(GLOB unittests "*_unittest.*")

list(REMOVE_ITEM sources ${unittests})

add_library(scada_common STATIC ${sources})

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/project.h.src"
  "project.h"
)

# Add project.h to includes.
target_include_directories(scada_common PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(scada_common PUBLIC
  base
  express
  scada_base
  scada_core
)

add_subdirectory(address_space)

# UT

add_executable(scada_common_unittests ${unittests})
target_link_libraries(scada_common_unittests PUBLIC
  scada_common
  gmock_main
  address_space
)
add_test(NAME scada_common_unittests COMMAND scada_common_unittests)

set_property(TARGET scada_common PROPERTY FOLDER ${scada_folder})
set_property(TARGET scada_common_unittests PROPERTY FOLDER ${scada_folder})
