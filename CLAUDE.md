# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

ScadaCommon is a standalone CMake subproject providing shared C++ libraries used by both the SCADA server and client. It is included by the parent project via `find_package(ScadaCommon)` using `FindScadaCommon.cmake` (a target-guarded `add_subdirectory` wrapper). Depends on `find_package(ScadaCore)` and `find_package(Express)`.

## Build Commands

Can be built standalone or as part of the parent SCADA project. Developers create a `CMakeUserPresets.json` (git-ignored) with local paths and MSVC environment — see `CMakeUserPresets.json.template`.

```shell
cmake --preset ninja-dev                                          # Configure
cmake --build --preset debug-dev                                  # Build (Debug)
cmake --build --preset release-dev                                # Build (RelWithDebInfo)
cmake --build --preset debug-dev --target scada_common            # Build specific target
ctest --preset test-debug-dev                                     # Run all tests (Debug)
ctest --preset test-release-dev                                   # Run all tests (RelWithDebInfo)
ctest --test-dir build/ninja-dev --build-config Debug --tests-regex timed_data  # Specific tests
```

## Modules and Dependencies

```
scada_core (external)          express (external)
    │                              │
    ├──────────┐                   │
    ▼          ▼                   │
address_space  scada_common ◄──────┘
    │              │
    │    ┌─────────┼──────────┐
    │    ▼         ▼          ▼
    │  events   node_service  timed_data
    │              │
    │    ┌────┬────┼────┐
    │    ▼    ▼    ▼    ▼
    │   v1   v2   v3  proxy
    │    │
    └────┘
```

| Module | CMake Target | Description |
|--------|-------------|-------------|
| `common/` | `scada_common` | Core utilities, data services, node state, expression engine |
| `address_space/` | `address_space` | OPC UA address space (nodes, types, hierarchy, builder) |
| `events/` | `scada_common_events` | Event storage, aggregation, per-node subscriptions |
| `node_service/` | `node_service` | Node service abstraction with async fetch, includes `static/` sources |
| `node_service/v1/` | `node_service_v1` | Address-space-based implementation with status tracking |
| `node_service/v2/` | `node_service_v2` | Alternative implementation |
| `node_service/v3/` | `node_service_v3` | Alternative implementation |
| `node_service/proxy/` | `node_service_proxy` | Proxy/forwarding pattern |
| `opcua/` | `scada_core_opcua` | OPC UA type conversions, server/session/subscription wrappers (requires OPCUAPP) |
| `timed_data/` | `timed_data` | Time-series data with views, aliases, and computed expressions |
| `opc/` | `scada_common_opc` | Classic COM-based OPC conversions (Windows only) |
| `vidicon/` | `scada_common_vidicon` | Vidicon telemetry integration (Windows only) |

## CMake Conventions

All modules use `scada_module()` macros from `core/scada_module.cmake`:

```cmake
scada_module(my_module)                          # Auto-discovers *.cpp/*.h, separates *_unittest.cpp
scada_module_sources(my_module PRIVATE "subdir") # Add sources from subdirectory
target_include_directories(my_module PUBLIC "..") # Standard: expose parent dir for #include "module/header.h"
```

- Source files named `*_unittest.cpp` are automatically compiled into a `<target>_unittests` executable (GoogleTest).
- Files named `*_mock.h` are excluded from the main library but available to tests.
- On Windows, files in a `win/` subdirectory are automatically included.

## Testing

Unit test targets are auto-generated per module (e.g., `scada_common_unittests`, `address_space_unittests`, `node_service_unittests`). Tests use GoogleTest with `gtest_discover_tests(DISCOVERY_MODE PRE_TEST)`.

Test fixtures and utilities live in `test/` subdirectories:
- `address_space/test/` — `test_address_space.h`, `test_matchers.h`
- `node_service/test/` — `test_node_model.h`
- `node_service/v1/test/` — `node_service_test_context.h`

The `node_service_unittests` target links all implementation variants (v1, v2, v3, proxy) to test them together.

## Key Architectural Patterns

**Service interfaces**: `common/master_data_services.h` aggregates service abstractions (AttributeService, ViewService, SessionService, MonitoredItemService, MethodService, HistoryService, NodeManagementService) defined in ScadaCore.

**Address space builder**: `address_space/scada_address_space.h` provides a builder pattern (`ScadaAddressSpaceBuilder`) for constructing the OPC UA node hierarchy. The address space uses an observer pattern for change notifications.

**Async node loading**: `node_service/node_ref.h` and `node_service/node_fetcher_impl.h` implement promise-based asynchronous node data fetching with status tracking and fetch queues.

**Time-series variants**: `timed_data/` provides multiple implementations — `base_timed_data`, `alias_timed_data`, `expression_timed_data` — behind a common `TimedData` interface with time-range view queries.

## Maintenance

- When adding, removing, or renaming presets in `CMakeUserPresets.json`, apply the same change to `CMakeUserPresets.json.template` (replacing local paths with placeholders) so the template stays in sync.
- When adding a new `find_package()` dependency, add its Find module directory to `CMAKE_MODULE_PATH` in both `CMakeUserPresets.json` and `CMakeUserPresets.json.template`.

## Include Conventions

All modules expose their parent directory as a public include path, enabling includes like:
```cpp
#include "common/node_state.h"
#include "address_space/node_utils.h"
#include "node_service/node_service.h"
#include "events/event_storage.h"
```
