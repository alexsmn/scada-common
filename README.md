# ScadaCommon

Shared C++ libraries for the Telecontrol SCADA client and server, providing the OPC UA address space, node services, event handling, and protocol type conversions.

## Modules

| Module | Description |
|--------|-------------|
| `common` | Core utilities, data services, node state, expression engine |
| `address_space` | OPC UA address space — nodes, types, hierarchy, builder |
| `events` | Event storage, aggregation, per-node subscriptions |
| `node_service` | Node service abstraction with v1/v2/v3 and proxy implementations |
| `opcua` | OPC UA type conversions, server/session/subscription wrappers |
| `timed_data` | Time-series data with views, aliases, and computed expressions |
| `opc` | Classic COM-based OPC conversions (Windows only) |
| `vidicon` | Vidicon telemetry integration (Windows only) |

## Building

ScadaCommon is typically built as part of the parent SCADA project via `find_package(ScadaCommon)`. It can also be configured standalone:

```shell
cmake --preset ninja
cmake --build build/ninja --config Debug
ctest --test-dir build/ninja --build-config Debug
```

Requires `VCPKG_ROOT` and `THIRD_PARTY` environment variables pointing to vcpkg and the third-party library root.

## License

Apache License 2.0 — see [LICENSE](LICENSE).
