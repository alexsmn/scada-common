scada_module(scada_core_vidicon)

target_link_libraries(scada_core_vidicon PUBLIC scada_core)

target_include_directories(scada_core_vidicon PUBLIC ".." "${deps}/OPCFoundation")
