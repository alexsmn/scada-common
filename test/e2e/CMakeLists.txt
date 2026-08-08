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
  "${CMAKE_CURRENT_LIST_DIR}/e2e_port_reservation.cpp"
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

# The machine-wide port claim, published on its own so a suite that wants ONLY
# that can take it without compiling the rest of the harness. The framework's
# per-tier startup E2E is exactly that case: it links nothing but gtest and
# boost, allocates a single port, and must still not collide with a client-suite
# cluster running from another checkout. Two variables rather than a target,
# matching SCADA_E2E_BUNDLED_SQLITE3 above, because the consumer is reached
# through find_package(ScadaServerFramework) rather than by directory order.
set(SCADA_E2E_PORT_RESERVATION_SOURCE
    "${CMAKE_CURRENT_LIST_DIR}/e2e_port_reservation.cpp"
    CACHE INTERNAL "Machine-wide E2E port claim implementation")
set(SCADA_E2E_HARNESS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
    CACHE INTERNAL "Include root for the shared E2E harness headers")

# The port claim is the one piece of the harness with its own unit tests, and
# the only one worth compiling unconditionally: it is what keeps concurrent E2E
# runs off each other's ports, it is pure enough to test without launching
# anything, and it runs on Linux — where none of the suites it protects do.
if(TARGET base_unittest)
  add_executable(scada_e2e_port_reservation_unittests
    "${CMAKE_CURRENT_LIST_DIR}/e2e_port_reservation.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/e2e_port_reservation_unittest.cpp"
  )
  target_include_directories(scada_e2e_port_reservation_unittests PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../.."
  )
  target_link_libraries(scada_e2e_port_reservation_unittests PRIVATE
    base_unittest
  )
  include(GoogleTest)
  gtest_discover_tests(scada_e2e_port_reservation_unittests
    DISCOVERY_MODE PRE_TEST)
endif()

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
