# The model library. The nodeset XML under nodesets/ is the single source of
# truth for the SCADA address space; the namespace and node-id constant headers
# are generated from it at build time (gen/generate_model_headers.py) rather
# than maintained by hand. Hand-written pieces (node_id utilities, the
# static-nodeset accessor, the security helpers, transitional shims) live
# alongside as normal sources.

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(_model_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/model")
set(_model_nodesets "${CMAKE_CURRENT_SOURCE_DIR}/nodesets")
set(_model_generator "${CMAKE_CURRENT_SOURCE_DIR}/gen/generate_model_headers.py")

file(GLOB _model_nodeset_xml CONFIGURE_DEPENDS "${_model_nodesets}/*.xml")

set(_model_generated
    "${_model_gen_dir}/namespaces.h"
    "${_model_gen_dir}/namespaces.cpp"
    "${_model_gen_dir}/scada_node_ids.h"
    "${_model_gen_dir}/data_items_node_ids.h"
    "${_model_gen_dir}/devices_node_ids.h"
    "${_model_gen_dir}/history_node_ids.h"
    "${_model_gen_dir}/security_node_ids.h"
    "${_model_gen_dir}/filesystem_node_ids.h"
    "${_model_gen_dir}/opc_node_ids.h")

add_custom_command(
  OUTPUT ${_model_generated}
  COMMAND ${CMAKE_COMMAND} -E make_directory "${_model_gen_dir}"
  COMMAND Python3::Interpreter "${_model_generator}"
          --nodesets "${_model_nodesets}" --out "${_model_gen_dir}"
  DEPENDS ${_model_generator} ${_model_nodeset_xml}
          "${_model_nodesets}/namespaces.csv"
          "${_model_nodesets}/extra_node_ids.csv"
          "${_model_nodesets}/code_domains.csv"
  COMMENT "Generating model namespace + node-id headers from nodesets"
  VERBATIM)

add_custom_target(scada_model_generate DEPENDS ${_model_generated})
set_source_files_properties(${_model_generated} PROPERTIES GENERATED TRUE)

scada_module(scada_model)
# Generation must run before any scada_model source (which include the generated
# model/*.h) compiles.
add_dependencies(scada_model scada_model_generate)
# The generated namespaces.cpp is not under the source dir, so add it explicitly.
target_sources(scada_model PRIVATE "${_model_gen_dir}/namespaces.cpp")

# Expose the common root (for the hand-written model/*.h) and the generated
# headers dir (for the generated model/*.h). Both use the "model/" prefix over
# disjoint filenames, so the search order is immaterial.
target_include_directories(scada_model PUBLIC ".." "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(scada_model PUBLIC scada_core)

# Modules-pilot consumer: node_id_util.cpp and namespaces.cpp import scada.core
# instead of including base/scada headers (see SCADA_USE_CORE_MODULE there).
scada_module_import_pilot(scada_model
  IMPORTS scada_core_module
  DEFINES SCADA_USE_CORE_MODULE)

scada_add_module_facade(scada_model FILES scada_model.cppm
  IMPORTS scada_core_module)
if(SCADA_CXX_MODULES)
  add_subdirectory(module_test)
endif()

if(TARGET scada_model_unittests)
  add_dependencies(scada_model_unittests scada_model_generate)
endif()
