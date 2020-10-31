file(GLOB sources
  "*.cpp"
  "*.h"
)

file(GLOB unittests
  "*_unittest.*")

list(REMOVE_ITEM sources ${unittests})

add_library(address_space_common STATIC ${sources})
target_include_directories(address_space_common PUBLIC "..")
target_link_libraries(address_space_common PUBLIC
  scada_common
  address_space
  node_service
)

add_executable(address_space_common_unittests ${unittests})
target_link_libraries(address_space_common_unittests PUBLIC
  address_space_common
  gmock_main
 )
add_test(NAME address_space_common_unittests COMMAND scada_common_unittests)

set_property(TARGET address_space_common PROPERTY FOLDER ${scada_folder})
set_property(TARGET address_space_common_unittests PROPERTY FOLDER ${scada_folder})
