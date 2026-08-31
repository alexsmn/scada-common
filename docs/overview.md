# Common Architecture

Status: Living reference
Last verified against code: 2026-08-31 (the "OPC UA Module" section only,
rewritten after that module left `common/` for three separate homes. The rest
of this index is unverified — this document has never carried a date, so treat
the other sections as unchecked rather than as recently confirmed.)

This document indexes the shared `common/` libraries used by both the SCADA
server and client.

Related documents:

- [../README.md](../README.md) for the top-level common-library overview
- [./opcua.md](./opcua.md) for the shared OPC UA transport and
  conversion layer
- [./node_service.md](./node_service.md) for the NodeService design
  (bounded residency, immutable NodeState snapshots)

## Modules

| Module | Purpose |
|--------|---------|
| `common/` | Core utilities, data services, node state, expression engine |
| `address_space/` | OPC UA address space nodes, hierarchy, and builder |
| `events/` | Event storage, aggregation, and event subscriptions |
| `node_service/` | Node access abstraction layers and proxy implementations |
| `opcua/` | OPC UA conversion layer plus client/server/session wrappers |
| `timed_data/` | Time-series data with aliases and computed expressions |
| `opc/` | Classic COM-based OPC conversions (Windows only) |
| `vidicon/` | Vidicon telemetry integration (Windows only) |

## OPC UA Module

The OPC UA boundary this section used to place in a single ~~`common/opcua/`~~
module is now split across three homes, and `common/` owns only the middle one:

| Where | What |
|---|---|
| `third_party/opcuapp/opcua/` | The self-contained UA stack: `binary::Server` for the `opc.tcp://` endpoint (`transport/binary/server.h`), the JSON-over-WebSocket transport (`transport/websocket/`), and `ClientSession` / `ClientSubscription` plus monitored-item plumbing for outbound sessions (`client/`). Namespaced `opcua::`, with no dependency on SCADA `core`. |
| `common/opcua_bridge/` | The adapter between the `scada::` and `opcua::` type universes: `conversion.h`, `service_conversion.h` and `vector_conversion.h` for the types; `server_adapters.h` to expose core services to opcuapp's server runtime; `client_adapters.h` to wrap an outbound `opcua::ClientSession` as core services, assembled by `CreateClientDataServices()`. See `common/opcua_bridge/README.md`. |
| `scada-server-framework/modules/opcua/` | `OpcUaModule` (`opcua_module.h`), which installs the server-side endpoint into a tier. |

`ExtensionObject` / `EventNotification` payloads are `std::any` and do not
cross the type boundary by value — the type id converts and the payload is left
empty, the wire codec carrying the body.

### Module Overview

![Common OPC UA module overview](./diagrams/opcua_module_overview.svg)

Source: [opcua_module_overview.puml](./diagrams/opcua_module_overview.puml)

### Server Request Flow

![Common OPC UA server request flow](./diagrams/opcua_server_request_flow.svg)

Source: [opcua_server_request_flow.puml](./diagrams/opcua_server_request_flow.puml)

### Client Session Flow

![Common OPC UA client session flow](./diagrams/opcua_client_session_flow.svg)

Source: [opcua_client_session_flow.puml](./diagrams/opcua_client_session_flow.puml)

See also: [opcua.md](./opcua.md)
