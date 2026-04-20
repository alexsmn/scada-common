# OPC UA Module Design

This document describes the design of the shared `common/opcua/` module.

Related documents:

- [design.md](./design.md) for the broader common-library index
- [../README.md](../README.md) for the top-level common-library overview

## Diagrams

### Module Overview

![Common OPC UA module overview](./opcua_module_overview.svg)

Source: [opcua_module_overview.mmd](./opcua_module_overview.mmd)

### Server Request Flow

![Common OPC UA server request flow](./opcua_server_request_flow.svg)

Source: [opcua_server_request_flow.mmd](./opcua_server_request_flow.mmd)

### Client Session Flow

![Common OPC UA client session flow](./opcua_client_session_flow.svg)

Source: [opcua_client_session_flow.mmd](./opcua_client_session_flow.mmd)

## Purpose

The shared OPC UA module supplies both sides of the project's classic UA TCP
integration:

- server-side endpoint hosting through `OpcUaServer`
- client-side outbound UA sessions through `OpcUaSession`
- conversion between OPC UA C-stack types and SCADA-native service types
- monitored-item and event subscription bridging
- a `DataServicesFactory` adapter that exposes an outbound UA session as the
  standard SCADA service interfaces

At runtime it sits between:

- the OPC Foundation ANSI C stack wrapped by `opcuapp`
- the shared SCADA service interfaces such as `AttributeService`,
  `ViewService`, `MethodService`, and `MonitoredItemService`
- the server module in `server/opcua/`
- client-side callers that want to talk to a remote OPC UA endpoint through
  the same service abstractions used elsewhere in the codebase

## Main Components

### `OpcUaServer`

Files:

- `common/opcua/opcua_server.h`
- `common/opcua/opcua_server.cpp`

Server-side bridge from OPC UA binary requests into the shared SCADA service
interfaces.

Responsibilities:

- open the `opc.tcp://` endpoint through `opcuapp::server::Endpoint`
- bind per-service request handlers for read, write, browse, translate, call,
  add-nodes, and delete-nodes
- adapt monitored-item creation into `scada::MonitoredItemService`
- keep the OPC UA stack configuration, certificate, and trace settings
  together in one runtime object

`OpcUaServer` intentionally does not implement business logic itself. It
converts OPC UA request payloads into SCADA-native request types, forwards
them into the supplied services, then converts the results back into OPC UA
responses.

### `OpcUaSession`

Files:

- `common/opcua/opcua_session.h`
- `common/opcua/opcua_session.cpp`

Client-side UA session adapter that implements the shared SCADA service
interfaces on top of an outbound OPC UA channel and session.

Responsibilities:

- connect a client channel and session to a remote `opc.tcp://` endpoint
- expose browse, read, write, method-call, and monitored-item operations
  through the shared service interfaces
- keep the default subscription alive for monitored-item traffic
- translate channel/session failures into the shared status model

The current implementation keeps reconnect and disconnect behavior minimal,
but it already establishes the core adapter boundary used by
`CreateOpcUaServices(...)`.

### `OpcUaSubscription`

Files:

- `common/opcua/opcua_subscription.h`
- `common/opcua/opcua_subscription.cpp`

Client-side monitored-item manager layered on one OPC UA subscription.

Responsibilities:

- create the default OPC UA subscription for `OpcUaSession`
- batch monitored-item subscribe and unsubscribe operations
- route data-change notifications into `scada::DataChangeHandler`
- route event notifications into `scada::EventHandler`
- propagate subscription-level failures back through the session error path

This is the layer that makes the shared `scada::MonitoredItem` abstraction
look like native OPC UA subscriptions and monitored items.

### `opcua_conversion.*`

Files:

- `common/opcua/opcua_conversion.h`
- `common/opcua/opcua_conversion.cpp`
- `common/opcua/opcua_conversion_unittest.cpp`

Type-conversion layer between OPC UA C-stack structs and SCADA-native types.

Responsibilities:

- convert scalar and structured values such as `Variant`, `DataValue`,
  `NodeId`, `ExpandedNodeId`, `QualifiedName`, and `LocalizedText`
- convert browse, method, node-management, monitoring, and event-filter
  payloads
- normalize status-code translation between the two type systems

This conversion layer is shared by both `OpcUaServer` and `OpcUaSession`, so
service logic and tests can stay on the SCADA-native type model.

### `CreateOpcUaServices(...)`

File:

- `common/opcua/opcua_services_factory.cpp`

Factory adapter that exposes one outbound `OpcUaSession` through the shared
`DataServices` bundle.

Responsibilities:

- construct `OpcUaSession`
- publish that session as the `session`, `view`, `attribute`, `method`, and
  `monitored-item` service surface
- shield callers from `OpcUaSession` construction failures

## Runtime Composition

The shared OPC UA pieces are wired in two main ways:

1. The server creates `server/opcua/OpcUaModule`.
2. `OpcUaModule` loads certificates and trace settings from `server.json`.
3. `OpcUaModule` constructs `OpcUaServer` with the shared root SCADA services.
4. `OpcUaServer` opens the `opc.tcp://` endpoint and translates inbound UA
   traffic into those services.

Separately:

1. A caller invokes `CreateOpcUaServices(...)`.
2. The factory constructs `OpcUaSession`.
3. `OpcUaSession` exposes browse, read, write, call, and monitored-item
   operations through the shared service interfaces.
4. `OpcUaSession` lazily creates `OpcUaSubscription` when monitored items are
   requested.

## Request Flows

### Server-side Request Handling

1. A UA client sends a binary request to the `opc.tcp://` endpoint.
2. `opcuapp::server::Endpoint` dispatches the request into `OpcUaServer`.
3. `OpcUaServer` converts the request payload into SCADA-native types.
4. The corresponding shared service performs the real read, write, browse,
   method, or node-management work.
5. `OpcUaServer` converts the result back into OPC UA response types.
6. The endpoint writes the response back to the UA client.

### Client-side Session Handling

1. A caller creates the shared data-services bundle through
   `CreateOpcUaServices(...)`.
2. `OpcUaSession::Connect(...)` opens a UA channel and then creates and
   activates a UA session.
3. Reads, writes, browse calls, and method calls use that active session.
4. The first monitored-item request lazily creates `OpcUaSubscription`.
5. The subscription batches monitored-item operations and forwards live
   updates back into the shared monitored-item handlers.

## Cross-Module Boundaries

- The server-side `server/opcua/OpcUaModule` owns configuration loading and
  endpoint lifecycle, while `common/opcua/OpcUaServer` owns protocol
  adaptation.
- Any future sibling transport should reuse the same SCADA service model
  rather than replacing `OpcUaServer`.
- The shared conversion layer is also the natural place to keep any future
  UA transport siblings consistent on SCADA-native type semantics.
