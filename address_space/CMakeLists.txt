file(GLOB sources CONFIGURE_DEPENDS
  "*.cpp"
  "*.h")

file(GLOB unittests CONFIGURE_DEPENDS
  "*_unittest.*"
  "*_mock.*")

list(REMOVE_ITEM sources ${unittests})

add_library(address_space STATIC ${sources})
target_include_directories(address_space PUBLIC "..")
target_link_libraries(address_space
  base
  express
  scada_common
  scada_model
)

add_executable(address_space_unittests ${unittests})
target_link_libraries(address_space_unittests
  address_space
  gmock_main
 )
add_test(NAME address_space_unittests COMMAND address_space_unittests)

set_property(TARGET address_space PROPERTY FOLDER ${scada_folder})
set_property(TARGET address_space_unittests PROPERTY FOLDER ${scada_folder})
