scada_module(scada_common_vidicon_services)

find_package(OPC REQUIRED)

target_link_libraries(scada_common_vidicon_services PUBLIC
  address_space
  scada_core
  scada_common_vidicon
  transport
)

target_link_libraries(scada_common_vidicon_services PRIVATE
  OPC::opc
)

target_include_directories(scada_common_vidicon_services PUBLIC "../..")
