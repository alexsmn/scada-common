file(GLOB sources
  "*.cpp"
  "*.h"
  "../timed_data/*.cpp"
  "../timed_data/*.h"
)

file(GLOB unittests
  "*_unittest.*")

list(REMOVE_ITEM sources ${unittests})

add_library(scada_common STATIC ${sources})
target_link_libraries(scada_common PUBLIC
  base
  express
  scada_base
  scada_core
)

# UT

add_executable(scada_common_unittests ${unittests})
target_link_libraries(scada_common_unittests PUBLIC
  scada_common
  gmock_main
)
add_test(NAME scada_common_unittests COMMAND scada_common_unittests)

set_property(TARGET scada_common PROPERTY FOLDER ${scada_folder})
set_property(TARGET scada_common_unittests PROPERTY FOLDER ${scada_folder})
