file(GLOB sources "*.cpp" "*.h")

add_library(scada_core_vidicon ${sources})

target_link_libraries(scada_core_vidicon scada_core)

target_include_directories(scada_core_vidicon PUBLIC "${deps}/OPCFoundation")

set_property(TARGET scada_core_vidicon PROPERTY FOLDER ${scada_folder})
