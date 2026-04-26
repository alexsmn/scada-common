# Common Coroutine Migration Plan

## Summary

Migrate `common/` incrementally to coroutine-first internals while preserving
the existing callback and `promise<T>` public APIs. New coroutine code should
use the shared repo utilities:

- `Awaitable<T>` and `CoSpawn(...)` from `base/awaitable.h`
- `ToPromise(...)` and `AwaitPromise(...)` from `base/awaitable_promise.h`
- `CallbackToAwaitable(...)` from `base/callback_awaitable.h`
- SCADA service helpers from `scada/service_awaitable.h`
- Coroutine SCADA service interfaces and adapters from
  `scada/coroutine_services.h`

Do not add common-specific async primitives.

## Rules

- Keep public common interfaces stable until downstream client and server code
  can move together.
- Prefer coroutine implementation helpers behind existing callback or promise
  boundaries.
- Preserve `AnyExecutor` affinity and existing shutdown/cancelation behavior.
- Use shared service awaitable helpers instead of open-coded
  callback-to-awaitable bridges.
- Prefer `scada::Coroutine*Service` interfaces from
  `core/scada/coroutine_services.h` for newly migrated internals. Use
  `CallbackToCoroutine*ServiceAdapter` at legacy callback service boundaries
  and `CoroutineToCallback*ServiceAdapter` only where downstream public APIs
  still require callback services.
- Treat adapters as compatibility boundaries, not as permanent layering for
  each class.

## Migration Slices

- `common/node_service`: migrate service request dispatch inside
  `NodeFetcherImpl` and `NodeChildrenFetcher` while preserving batching,
  request IDs, late-response cancellation, queued-child cancellation, and
  fetched-node notification behavior. New request pipelines should depend on
  coroutine SCADA service interfaces where practical, adapting legacy
  `AttributeService` and `ViewService` dependencies at module boundaries.
- `common/timed_data`: migrate history-read gaps to coroutine helpers while
  preserving continuation-point cleanup. Move internal history access toward
  `CoroutineHistoryService`; keep `HistoryService` adapters only at public
  callback-facing boundaries.
- `common/events`: migrate event history refresh and acknowledgement calls to
  coroutine tasks while preserving observer, queueing, and max-parallel-ack
  behavior. Event history and acknowledgement internals should consume
  coroutine service interfaces once their owning services can supply them.
- `common/opcua`: keep existing binary/client coroutine code as the reference
  pattern and remove local bridges where shared service awaitable helpers
  already cover the same boundary. Legacy callback entry points should only
  schedule coroutine work through a weak-session dispatch point so queued
  callbacks do not touch torn-down client state. Prefer coroutine SCADA service
  adapters for server/runtime dependencies rather than wrapping individual
  callback methods at each call site.

## Coroutine Service Migration

The next stage is to migrate common internals from callback SCADA service
interfaces (`AttributeService`, `HistoryService`, `ViewService`,
`MethodService`, `NodeManagementService`, and `SessionService`) to the
coroutine service interfaces in `core/scada/coroutine_services.h`.

- Use `CoroutineAttributeService`, `CoroutineHistoryService`,
  `CoroutineViewService`, `CoroutineMethodService`,
  `CoroutineNodeManagementService`, and `CoroutineSessionService` as internal
  dependencies for migrated code.
- Use `CallbackToCoroutine*ServiceAdapter` to wrap existing callback services
  when a module has not yet been converted end to end.
- Use `CoroutineToCallback*ServiceAdapter` to preserve existing public callback
  APIs while the implementation behind them is coroutine-first.
- Keep `scada/service_awaitable.h` helpers for narrow transitional call sites,
  but do not add new local callback-to-awaitable wrappers in `common/`.
- Tests for each converted module should cover both synchronous callback
  completion through adapters and delayed executor completion, so adapter
  scheduling and lifetime behavior stay explicit.

## Verification

Build and run the focused common unit targets:

- `node_service_unittests`
- `timed_data_unittests`
- `scada_common_events_unittests`
- `scada_core_opcua_unittests`

Add targeted tests for synchronous service completion, late response
cancelation, continuation-point cleanup, and event ack queue parallelism as
each slice is touched.

## Status

- `NodeFetcherImpl` now consumes `CoroutineAttributeService` and
  `CoroutineViewService` internally; v1/v2 node-service owners adapt legacy
  callback services at construction boundaries, and tests cover synchronous and
  delayed adapter-backed fetch completion.
- `NodeChildrenFetcher` now consumes `CoroutineViewService` internally; v1/v2
  owners adapt legacy callback view services at construction boundaries, and
  tests cover delayed completion, merge behavior, and queued-child
  cancellation.
- `TimedDataFetcher` and `ScopedContinuationPoint` now consume
  `CoroutineHistoryService` internally, with callback history services adapted
  once at construction boundaries; timed-data tests cover cleanup dispatch,
  explicit continuation-point release, and adapter-backed completion.
- `EventFetcher` history refresh now consumes `CoroutineHistoryService`, and
  `EventAckQueue` owns a coroutine method-service adapter for acknowledgement
  dispatch. Events tests cover adapter-backed history refresh and
  acknowledgement dispatch, duplicate suppression, and max-parallel scheduling.
- `EventFetcher` monitored-item event delivery now posts through coroutine
  tasks guarded by the fetcher's cancellation token; tests cover executor
  delivery, destroyed-fetcher suppression, and failed notification status
  suppression before event storage is touched.
- `ClientSession` legacy view, attribute, and method callbacks now share a
  weak coroutine dispatch helper instead of capturing raw session state in each
  wrapper; OPC UA tests cover all callback wrappers and destroyed-session
  callback suppression.
- `ClientSession` now exposes an owned `CoroutineSessionService` facade over
  its coroutine lifecycle methods and session metadata, keeping legacy
  `SessionService` inheritance separate from the conflicting coroutine
  lifecycle method names; OPC UA tests cover invalid endpoint rejection,
  disconnected metadata, session-state subscription, and successful connect
  through the facade.
- `ClientSubscription` lazy subscription and monitored-item operations now run
  through weak coroutine tasks that keep subscription state alive across awaits;
  pending monitored-item subscriptions are queued until server subscription
  creation completes, and OPC UA tests cover the public client monitored-item
  create/delete flow.
- `ServerRuntime` publish-delay waiting now bridges delayed callbacks through
  `AwaitPromise` instead of a local `CallbackToAwaitable` adapter; OPC UA tests
  cover the injected scheduler callback path.
- `ServiceHandler` now consumes coroutine attribute, view, history, method,
  and node-management services directly and no longer uses per-call
  `scada/service_awaitable.h` helpers. `ServerRuntime` adapts its callback
  service dependencies once at construction; OPC UA tests cover the canonical,
  websocket, server-runtime, and binary dispatch paths.
- Event and node-service channel notifiers now consume
  `CoroutineSessionService`; their builders adapt the public `SessionService`
  boundary once with `PromiseToCoroutineSessionServiceAdapter`, and common
  tests cover connected-session startup plus later session-state callbacks.
- `MasterDataServices` can now be constructed with an executor to expose
  coroutine attribute, view, history, method, and node-management services over
  its aggregate `DataServices` boundary; callback/promise public methods keep
  their existing API while dispatching through the coroutine adapters when
  available, and common tests cover delayed adapter completion for callback,
  coroutine, and session-promise paths.
- `MasterDataServices` now also exposes an owned `CoroutineSessionService`
  facade over its aggregate session boundary, reusing the existing coroutine
  session adapter when a legacy `SessionService` is installed; common tests
  cover delayed promise-backed connect completion and session metadata
  delegation through the facade.
- `Audit` now exposes coroutine attribute/view services and can wrap audited
  `scada::services` with an executor so callback entry points dispatch through
  coroutine adapters while preserving read/browse metric accounting; common
  tests cover delayed callback completion, coroutine browse, and audited-service
  wrapping.
- `EventAckQueue` now consumes `CoroutineMethodService` directly; the event
  fetcher builder and tests adapt callback method services at construction
  boundaries, keeping acknowledgement scheduling coroutine-first.
- `EventAckQueue` pending acknowledgement dispatch is now cancellation-bound
  before it touches queue state, so destroying the queue suppresses posted ack
  drains as well as in-flight coroutine method calls; events tests cover the
  destroyed-queue suppression path.
- Node-service v1 `AddressSpaceFetcherImpl` and v2 `NodeServiceImpl` now
  receive coroutine attribute/view services directly; their factories and tests
  own the callback-to-coroutine adapters at construction boundaries.
- `TimedDataServiceImpl` now owns the history callback-to-coroutine adapter
  and passes a shared `CoroutineHistoryService` dependency through
  `TimedDataContext`; `TimedDataImpl` no longer creates per-instance history
  adapters.
- Removed stale callback attribute, method, and monitored-item service
  dependencies from v1 `NodeServiceImpl`/`NodeModelImpl`; v1 node models now
  keep only node state and the public `scada::node` handle they actually use.
- Local address-space attribute and view service implementations now expose
  `CoroutineAttributeService` and `CoroutineViewService` directly while
  preserving their callback service APIs; coroutine tests cover read, write,
  browse, and browse-path translation against the same sync service core.
- Local address-space method and node-management service stubs now expose
  `CoroutineMethodService` and `CoroutineNodeManagementService` directly while
  preserving their callback APIs; coroutine tests cover the method call and all
  node-management mutation stubs.
- Local address-space history service now exposes `CoroutineHistoryService`
  directly while preserving the callback API; coroutine tests cover generated
  raw profiles and stored event reads.
- Local address-space session fixtures now have a direct
  `LocalCoroutineSessionService` implementation for connected in-memory
  sessions; coroutine tests cover lifecycle completion and local session
  metadata without routing through a promise adapter.
- Address-space method service implementation now exposes
  `CoroutineMethodService` directly while preserving its callback helper;
  coroutine tests cover the built-in wrong-method-id result.
- Vidicon session services now expose coroutine attribute, view, history,
  method, and node-management interfaces directly plus an owned
  `CoroutineSessionService` facade while preserving their callback APIs; the
  services target publishes the address-space, Vidicon IDL, and transport
  dependencies needed by the public coroutine-facing header, and Vidicon
  service tests cover local session metadata, lifecycle, local read/browse,
  and unsupported write, history, method, and node-management coroutine paths.
- OPC UA `ServerRuntime` and binary `Runtime` now accept coroutine service
  contexts directly while retaining legacy callback contexts that own the
  compatibility adapters; runtime tests cover canonical and binary read
  dispatch through coroutine services without constructing callback adapters.
- OPC UA websocket `Server` tests now exercise `CoroutineServerRuntimeContext`
  end to end, covering CreateSession/ActivateSession/Read over the websocket
  transport loop through direct coroutine SCADA services without callback
  service adapters.
- Node-service v1/v2 factories now expose a coroutine-native construction path
  for attribute/view services while preserving the legacy callback-service
  factory path; factory tests cover connected-session fetches through direct
  coroutine services for both implementations.
- Node-service v1/v2 coroutine factories now also consume
  `CoroutineSessionService` directly, leaving `SessionService` adaptation only
  on the legacy factory path; factory tests verify connected startup and node
  fetches through direct coroutine session, attribute, and view services.
- Removed the stale `MethodService` dependency from the node-service coroutine
  factory context; the direct coroutine path now carries only the services that
  node fetching actually uses, and factory tests no longer need callback
  method/session mocks for coroutine construction.
- v1 `AddressSpaceMethodService` now exposes `CoroutineMethodService` directly
  while preserving the legacy callback method API; node-service tests cover the
  unsupported-method result through both callback and coroutine calls.
- `EventFetcherBuilder` now has a coroutine-native construction path for
  session, history, and method services while the legacy builder keeps adapter
  ownership at the callback-service boundary; events tests cover connected
  startup history refresh and acknowledgement dispatch through direct coroutine
  services.
- `TimedDataService` factories now expose a coroutine-native construction path
  for `CoroutineHistoryService` while the legacy factory keeps callback adapter
  ownership at the service boundary; timed-data tests cover range history fetch
  through direct coroutine services.
- Removed stale callback-service dispatch helpers from OPC UA
  `endpoint_core`; service routing now lives only in coroutine
  `ServiceHandler`, while endpoint-core tests cover the remaining
  normalization, monitored-item, and event-field helpers.
- The aggregate `DataServices` boundary now carries explicit coroutine SCADA
  service slots in addition to the legacy callback slots; OPC UA and Vidicon
  factories populate direct coroutine services, and `MasterDataServices`
  prefers those slots before callback dynamic-casts or adapters. Common tests
  cover callback and coroutine aggregate APIs with only direct coroutine slots
  installed.
- `EventFetcherBuilder` now accepts the coroutine-capable `DataServices`
  aggregate and resolves direct coroutine session/history/method services
  before falling back to legacy callback adapters; the legacy `scada::services`
  field remains as a compatibility path. Events tests cover history refresh
  through the non-coroutine builder with only direct coroutine service slots
  installed.
- `TimedDataServiceImpl` now also accepts the coroutine-capable `DataServices`
  aggregate and resolves direct `CoroutineHistoryService` slots before
  callback history adapters; timed-data tests cover range history fetch through
  the legacy timed-data factory context with only a direct coroutine history
  slot installed.
- `Audit` now also accepts the coroutine-capable `DataServices` aggregate and
  the new `AuditDataServices` helper preserves audited callback and coroutine
  attribute/view slots; common tests cover audited callback read and coroutine
  browse dispatch with only direct coroutine services installed.
