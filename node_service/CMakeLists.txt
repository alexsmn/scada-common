scada_module(node_service)
scada_module_sources(node_service PRIVATE "static")

# No PCH in the modules pilot (see core docs: scan-deps cannot read it).
if(NOT SCADA_CXX_MODULES)
  target_precompile_headers(node_service REUSE_FROM scada_base)
endif()

target_include_directories(node_service PUBLIC "..")

target_link_libraries(node_service PUBLIC
 scada_common
)

target_link_libraries(node_service PRIVATE
  transport
)

# Modules-pilot consumer: node_ref.cpp imports scada.core (see
# SCADA_USE_CORE_MODULE there).
scada_module_import_pilot(node_service
  IMPORTS scada_core_module
  DEFINES SCADA_USE_CORE_MODULE)

scada_add_module_facade(node_service FILES scada_node_service.cppm
  IMPORTS scada_common_module)
if(SCADA_CXX_MODULES)
  add_subdirectory(module_test)
endif()

# Implementations.

add_subdirectory(v3)
add_subdirectory(proxy)

# node_service_factory.cpp, compiled into node_service, constructs the v3
# implementation, which itself links node_service: a genuine cycle between two
# static libraries. Declared, CMake repeats them on the link line; undeclared,
# only a linker that rescans archives (macOS's) resolved it, and GNU ld did
# not (undefined v3::CreateNodeService, scada-client run 36304111222).
target_link_libraries(node_service PRIVATE node_service_v3)

# Link implementations to common UTs.

target_link_libraries(node_service_unittests PUBLIC
  node_service_v3
  node_service_proxy
  # The test fixtures build a real address space (TestAddressSpace,
  # AddressSpaceImpl, ViewServiceImpl) and MakeNodeStates. This used to be
  # pulled in transitively through node_service_v1 (the only impl that kept an
  # address-space mirror); link it directly now that v1 is gone.
  address_space
)

if(MSVC)
  target_link_options(node_service_unittests PRIVATE
    $<$<CONFIG:Debug>:/INCREMENTAL:NO>
  )
endif()
