scada_module(scada_common_events)

target_precompile_headers(scada_common_events REUSE_FROM scada_base)

target_include_directories(scada_common_events PUBLIC "..")

target_link_libraries(scada_common_events PUBLIC
  scada_common
  transport
)
