# The shared E2E cluster harness, exposed as targets rather than as a directory
# consumers name file by file.
#
# Both consumers — client/test/e2e and the superproject's service x namespace
# sweep — used to list these sources, this include directory and these fixture
# paths themselves, each prefixed with ${CMAKE_SOURCE_DIR}/common. That is the
# top-level source directory, so it resolved correctly only while the
# superproject was the top-level project, and it duplicated the same six-file
# list in two places that then drifted apart (the client omits the span
# collector; the sweep needs it).
#
# INTERFACE, deliberately: common is built by the framework, every tier and the
# client, and none of them should compile an E2E harness they never link. An
# INTERFACE library costs nothing until something links it, and then carries its
# sources, include root and fixture paths with it.

add_library(scada_e2e_harness INTERFACE)

target_sources(scada_e2e_harness INTERFACE
  "${CMAKE_CURRENT_LIST_DIR}/e2e_cluster.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/e2e_file_helpers.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/e2e_process.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/e2e_server_process.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/iec61850_test_server.cpp"
)

# Consumers include these as "test/e2e/e2e_cluster.h", so the include root is
# the repository root, two directories up.
target_include_directories(scada_e2e_harness INTERFACE
  "${CMAKE_CURRENT_LIST_DIR}/../.."
)

# The fixture data lives here, so this is where its paths are known. Which tier
# binaries a suite drives is the consumer's business and stays with the
# consumer; where common's own fixtures are is not.
target_compile_definitions(scada_e2e_harness INTERFACE
  SCADA_E2E_SERVER_FIXTURE_DIR="${CMAKE_CURRENT_LIST_DIR}/fixtures/server-data"
  SCADA_E2E_SERVER_SETTINGS_TEMPLATE="${CMAKE_CURRENT_LIST_DIR}/fixtures/server-settings.json"
  SCADA_E2E_CONFIGURATION_BASE_SQL="${CMAKE_CURRENT_LIST_DIR}/fixtures/configuration-base.sql"
  SCADA_E2E_CONFIGURATION_FIXTURE_SQL="${CMAKE_CURRENT_LIST_DIR}/fixtures/configuration-fixture.sql"
)

# The bundled sqlite3 the Windows harness shells out to.
set(SCADA_E2E_BUNDLED_SQLITE3 "${CMAKE_CURRENT_LIST_DIR}/fixtures/sqlite3.exe"
    CACHE INTERNAL "Bundled sqlite3.exe used by the E2E harness on Windows")

# Span collection is separate: only the superproject's routing sweep asserts on
# spans, and pulling the OTLP receiver into every harness consumer would be a
# cost the client's suite has no use for.
add_library(scada_e2e_span_collector INTERFACE)
target_sources(scada_e2e_span_collector INTERFACE
  "${CMAKE_CURRENT_LIST_DIR}/e2e_span_collector.cpp"
)
target_include_directories(scada_e2e_span_collector INTERFACE
  "${CMAKE_CURRENT_LIST_DIR}/../.."
)
