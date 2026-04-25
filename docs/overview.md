# Common Architecture

This document indexes the shared `common/` libraries used by both the SCADA
server and client.

Related documents:

- [../README.md](../README.md) for the top-level common-library overview
- [./opcua/module.md](./opcua/module.md) for the shared OPC UA transport and
  conversion layer

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

The `common/opcua/` module is the shared OPC UA boundary. It provides:

- `OpcUaModule` plus `binary::Server` for the server-side `opc.tcp://` endpoint
- `ClientSession` for client-side outbound UA sessions
- `ClientSubscription` and monitored-item plumbing for live updates
- `opcua_conversion.*` for converting between OPC UA C-stack types and
  SCADA-native types
- `CreateServices(...)` for exposing an outbound UA session through the
  shared SCADA service interfaces

### Module Overview

![Common OPC UA module overview](./diagrams/opcua_module_overview.svg)

Source: [opcua_module_overview.mmd](./diagrams/opcua_module_overview.mmd)

### Server Request Flow

![Common OPC UA server request flow](./diagrams/opcua_server_request_flow.svg)

Source: [opcua_server_request_flow.mmd](./diagrams/opcua_server_request_flow.mmd)

### Client Session Flow

![Common OPC UA client session flow](./diagrams/opcua_client_session_flow.svg)

Source: [opcua_client_session_flow.mmd](./diagrams/opcua_client_session_flow.mmd)

See also: [opcua/module.md](./opcua/module.md)
