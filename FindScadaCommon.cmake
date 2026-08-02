# Where this repository lives, for consumers that need paths into it rather than
# just its targets — the shared E2E harness sources and the vds_runtime_api.h
# interface header. They used to reach here via ${CMAKE_SOURCE_DIR}/common,
# which is the top-level source directory: correct only when the superproject is
# the top-level project, and silently pointing at the consumer's own common/
# subdirectory otherwise.
set(SCADA_COMMON_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}"
    CACHE INTERNAL "Source directory of the scada-common repository")

if(NOT TARGET scada_common)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR} scada-common)
endif()