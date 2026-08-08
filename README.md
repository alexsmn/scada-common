# ScadaCommon

Shared C++ libraries for the Telecontrol SCADA client and server, providing the OPC UA address space, node services, event handling, and protocol type conversions.

Design docs:

- [common/docs/overview.md](./docs/overview.md)
- [common/docs/opcua.md](./docs/opcua.md)

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

ScadaCommon builds standalone, and is also spliced into any product that
consumes it via `find_package(ScadaCommon)`.

It consumes three other products — `core`, `express` and `opcuapp` — which must
be checked out beside it in a standalone checkout; the resolver in
`build-support/` finds them there, and in the SCADA monorepo where two of them
live under `third_party/`.

```shell
# Configure
cmake --preset ninja

# Build
cmake --build --preset release      # or: debug, relwithdebinfo

# Run tests
ctest --preset test-release         # or: test-debug
```

Every product in the SCADA tree carries this same preset set (ADR 0011), so the
commands do not change from one to the next. Set `VCPKG_ROOT` in the
environment; anything else machine-specific goes in `.scada-local.cmake` beside
`build-support/`. Output lands in `build/ninja/bin/<config>/`.

On Windows the `opc/` and `vidicon/` subtrees additionally need genuine
out-of-tree SDKs (OPC Foundation, midl, the Classic OPC client). Those are not
products, so name their directories in `SCADA_EXTRA_MODULE_PATH` in the machine
config — see `build-support/scada-local.cmake.example`.

## License

Apache License 2.0 — see [LICENSE](LICENSE).
