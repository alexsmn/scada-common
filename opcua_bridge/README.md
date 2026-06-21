# opcua_bridge

The boundary adapter between the SCADA `core` types (`scada::`) and the
extracted **opcuapp** types (`opcua::scada::`). It lets the SCADA monorepo
consume opcuapp (which is namespaced apart so both type universes coexist in one
translation unit) without changing core.

## Pieces

| File | Purpose |
|------|---------|
| `conversion.{h,cpp}` | Foundational converters: NodeId, ExpandedNodeId, QualifiedName, Status/StatusCode, DateTime, Qualifier, Variant (scalars + arrays + ExtensionObject), DataValue. |
| `service_conversion.{h,cpp}` | Service-interface structs: ReadValueId, WriteValue, Browse*, RelativePath, BrowsePath*, AddNodes/Delete/References, NodeAttributes, MonitoringParameters (+ filters), Event, history, monitored-item types, ServiceContext, SessionConnectParams, AuthenticationResult, and the enums. |
| `vector_conversion.h` | `ToOpcuaVector`/`ToScadaVector` + `StatusOr<vector<T>>` helpers; included after all converters so lookup sees them. |
| `server_adapters.{h,cpp}` | Wrap core `scada::*Service` impls as the `opcua::scada::*Service` opcuapp's server runtime consumes (Attribute, View, Method, NodeManagement, History, MonitoredItem + subscription, Authenticator). `ServerServiceAdapters` bundles them. |
| `client_adapters.{h,cpp}` | Inverse: wrap opcuapp's `opcua::ClientSession` as core services, assembled into a `::DataServices` by `CreateClientDataServices()`. |
| `*_unittest.cpp` | Round-trip conversion tests + a runtime adapter test (drives a coroutine through an adapter against a fake core service). |

## Design notes

- Most std-alias types (`String`=std::string, `LocalizedText`=std::u16string,
  `ByteString`=std::vector<char>, numeric primitives) are the **same** type on
  both sides and need no conversion — only class types, `base::Time`-backed
  `DateTime`/`Duration`, and enums do.
- `Awaitable` is the same `boost::asio::awaitable` on both sides, so an adapter
  coroutine can `co_await` the inner service's coroutine directly.
- Known limitation: `ExtensionObject` / `EventNotification` payloads are
  `std::any` and cannot cross the type boundary by value; the type id converts,
  the payload is left empty (the wire codec carries the body).

## Building / testing

Requires `find_package(opcuapp)` on `CMAKE_MODULE_PATH`. opcuapp's include
directory must precede `common/` (both provide an `opcua/` tree until the
in-tree native stack is removed) — the CMakeLists handles this with
`target_include_directories(... BEFORE ...)`.

The suite can also be built standalone against prebuilt libs (see the
`-I third_party/opcuapp -I common -I core` ordering); linking
`libscada_core.a` + `libscada_base.a` + `libopcuapp.a` + GoogleTest runs the
round-trip and adapter tests.
