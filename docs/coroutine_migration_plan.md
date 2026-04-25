# Common Coroutine Migration Plan

## Summary

Migrate `common/` incrementally to coroutine-first internals while preserving
the existing callback and `promise<T>` public APIs. New coroutine code should
use the shared repo utilities:

- `Awaitable<T>` and `CoSpawn(...)` from `base/awaitable.h`
- `ToPromise(...)` and `AwaitPromise(...)` from `base/awaitable_promise.h`
- `CallbackToAwaitable(...)` from `base/callback_awaitable.h`
- SCADA service helpers from `scada/service_awaitable.h`

Do not add common-specific async primitives.

## Rules

- Keep public common interfaces stable until downstream client and server code
  can move together.
- Prefer coroutine implementation helpers behind existing callback or promise
  boundaries.
- Preserve `AnyExecutor` affinity and existing shutdown/cancelation behavior.
- Use shared service awaitable helpers instead of open-coded
  callback-to-awaitable bridges.
- Treat adapters as compatibility boundaries, not as permanent layering for
  each class.

## Migration Slices

- `common/node_service`: migrate service request dispatch inside
  `NodeFetcherImpl` and `NodeChildrenFetcher` while preserving batching,
  request IDs, late-response cancellation, queued-child cancellation, and
  fetched-node notification behavior.
- `common/timed_data`: migrate history-read gaps to coroutine helpers while
  preserving continuation-point cleanup.
- `common/events`: migrate event history refresh and acknowledgement calls to
  coroutine tasks while preserving observer, queueing, and max-parallel-ack
  behavior.
- `common/opcua`: keep existing binary/client coroutine code as the reference
  pattern and remove local bridges where shared service awaitable helpers
  already cover the same boundary. Legacy callback entry points should only
  schedule coroutine work through a weak-session dispatch point so queued
  callbacks do not touch torn-down client state.

## Verification

Build and run the focused common unit targets:

- `node_service_unittests`
- `timed_data_unittests`
- `scada_core_opcua_unittests`

Add targeted tests for synchronous service completion, late response
cancelation, continuation-point cleanup, and event ack queue parallelism as
each slice is touched.

## Status

- `NodeFetcherImpl` request completion handling now runs through coroutine
  continuations while keeping the legacy upstream request timing observable to
  callers and tests.
- `NodeChildrenFetcher` now routes browse completion handling through a
  coroutine continuation and has coverage for delayed completion, merge
  behavior, and queued-child cancellation.
- `TimedDataFetcher` history reads now use coroutine tasks while preserving
  continuation-point ownership.
- `EventFetcher` history refresh and `EventAckQueue` acknowledgement dispatch
  now use coroutine tasks; `EventAckQueue` has coverage for dispatch,
  duplicate suppression, and max-parallel scheduling.
- `EventFetcher` monitored-item event delivery now posts through coroutine
  tasks guarded by the fetcher's cancellation token; tests cover executor
  delivery and destroyed-fetcher suppression.
- `ClientSession` legacy view, attribute, and method callbacks now share a
  weak coroutine dispatch helper instead of capturing raw session state in each
  wrapper; OPC UA tests cover all callback wrappers and destroyed-session
  callback suppression.
- `ServerRuntime` publish-delay waiting now bridges delayed callbacks through
  `AwaitPromise` instead of a local `CallbackToAwaitable` adapter; OPC UA tests
  cover the injected scheduler callback path.
