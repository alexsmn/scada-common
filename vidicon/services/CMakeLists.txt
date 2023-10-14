scada_module(scada_common_vidicon_services)

find_package(OPC REQUIRED)

target_link_libraries(scada_common_vidicon_services PUBLIC
  scada_core
)

target_link_libraries(scada_common_vidicon_services PRIVATE
  OPC::opc
  scada_common_vidicon
)

target_include_directories(scada_common_vidicon_services PUBLIC "../..")
