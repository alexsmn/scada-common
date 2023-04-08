scada_module(scada_core_vidicon)

find_package(OPC REQUIRED)

target_link_libraries(scada_core_vidicon PUBLIC scada_core OPC::opc)

target_include_directories(scada_core_vidicon PUBLIC "..")
