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
  delivery and destroyed-fetcher suppression.
- `ClientSession` legacy view, attribute, and method callbacks now share a
  weak coroutine dispatch helper instead of capturing raw session state in each
  wrapper; OPC UA tests cover all callback wrappers and destroyed-session
  callback suppression.
- `ClientSubscription` lazy subscription and monitored-item operations now run
  through weak coroutine tasks that keep subscription state alive across awaits;
  pending monitored-item subscriptions are queued until server subscription
  creation completes, and OPC UA tests cover the public client monitored-item
  create/delete flow.
- `ServerRuntime` publish-delay waiting now bridges delayed callbacks through
  `AwaitPromise` instead of a local `CallbackToAwaitable` adapter; OPC UA tests
  cover the injected scheduler callback path.
- `ServiceHandler` now owns coroutine adapters for attribute, view, history,
  method, and node-management services and no longer uses per-call
  `scada/service_awaitable.h` helpers; OPC UA tests cover the canonical,
  websocket, server-runtime, and binary dispatch paths.
