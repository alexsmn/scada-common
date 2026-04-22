# OPC UA Binary and WS Unification Plan

> Status: planning document for the remaining common-layer refactor.
> Phase 2 runtime ownership, Phase 3 service-handler ownership, and the
> canonical shared session / subscription / session-manager ownership move are
> now implemented. Binary adapter thinning has also advanced: the Binary
> dispatcher now handles all session and authenticated service requests
> directly through the shared runtime, with only Binary header/session-token
> adaptation and the `HistoryReadEvents` response-encoding special case
> remaining outside the core. The remaining work is primarily finishing Binary
> wire-only cleanup and broadening cross-transport parity tests. The WS
> UA-JSON codec/runtime boundary now also consumes and produces the canonical
> `opcua::` request/response/envelope types directly, with `OpcUaWs*` names
> retained only as compatibility aliases.

This document records the remaining unification gaps between the in-repo OPC UA
Binary stack in `common/opcua/` and the OPC UA over WebSocket stack in
`common/opcua_ws/`, then turns those gaps into a gradual implementation plan.

Related documents:

- [opcua_module.md](./opcua_module.md) for the current `common/opcua/` design
- [opcua_ws.md](./opcua_ws.md) for the current `common/opcua_ws/` design
- [../opcua/CLAUDE.md](../opcua/CLAUDE.md) for the binary schema source of truth

## Goal

Make OPC UA Binary and OPC UA WS two transport adapters over one canonical
common-layer server runtime, without forcing the transports to share framing,
handshake, or wire codecs.

In practice that means:

- keep transport-specific concerns in Binary TCP / SecureChannel and WS / WSS
- keep encoding-specific concerns in the Binary codec and UA-JSON codec
- move session lifecycle, subscription ownership, publish behavior, browse
  continuation handling, and service dispatch onto one transport-neutral model

## Current baseline

The codebase has already completed the first unification step:

- `common/opcua_ws/opcua_ws_session_manager.h` now defines the canonical
  session lifecycle types in `namespace opcua`, with `opcua_ws` aliases layered
  on top
- `common/opcua/opcua_runtime.{h,cpp}` now own the shared
  `opcua::OpcUaRuntime`, with `common/opcua_ws/opcua_ws_runtime.h` layering
  the compatibility alias back into `opcua_ws`
- `common/opcua_ws/opcua_ws_session.h` defines a shared `opcua::OpcUaSession`
  and aliases it back into `opcua_ws`
- `common/opcua/opcua_server_session_manager.{h,cpp}`,
  `common/opcua/opcua_server_session.{h,cpp}`, and
  `common/opcua/opcua_server_subscription.{h,cpp}` now physically own the
  canonical shared server-side session / subscription implementation, with the
  `common/opcua_ws/*` headers reduced to compatibility wrappers
- `common/opcua/opcua_binary_runtime.h` already reuses that shared runtime and
  explicitly describes it as the canonical server-side core
- `common/opcua/opcua_message.h` and `common/opcua/opcua_service_message.h`
  now own the canonical transport-neutral request/response model, with WS
  alias headers layered back on top
- `common/opcua/opcua_service_handler.{h,cpp}` now own the canonical coroutine
  service dispatcher, while the WS module retains only adapter-facing alias
  surfaces and transport/codec code
- `common/opcua_ws/opcua_json_codec.{h,cpp}` and
  `common/opcua_ws/opcua_ws_message_codec.cpp` now consume and produce the
  canonical `opcua::OpcUaServiceRequest`, `opcua::OpcUaServiceResponse`,
  `opcua::OpcUaRequestMessage`, and `opcua::OpcUaResponseMessage` types
  directly, with `OpcUaWs*` spellings left as compatibility aliases

So the direction is correct. The remaining work is mostly about removing the
last WS-shaped session/subscription naming from the shared core, finishing the
physical module ownership move, and making the Binary adapter thinner still.

## Unification gaps

### 1. Shared types are still WS-owned

The common runtime and session core live in `namespace opcua`, but many of the
canonical request/response types still originate from
`common/opcua_ws/opcua_ws_message.h` and are imported upward.

Examples still to clean up:

- `common/opcua/opcua_binary_message.h` aliases almost all Binary service types
  directly to `opcua_ws::*`
- some WS compatibility headers still re-export the canonical model under
  `OpcUaWs*` names for downstream compatibility

Impact:

- Binary currently depends on WS-defined service types instead of a transport-
  neutral common schema
- ownership is inverted: WS looks like the source module even when the logic is
  now shared

### 2. The runtime contract is still WS envelope shaped

The shared runtime itself is now canonical, and the WS UA-JSON codec/server
boundary now uses canonical request/response/envelope types directly. Binary
still adapts itself through
`common/opcua/opcua_binary_runtime.cpp`.

Impact:

- Binary is a second-class adapter around a WS-first core instead of a peer

### 3. Binary request dispatch still duplicates adapter logic

`common/opcua/opcua_binary_service_dispatcher.cpp` still contains a
Binary-specific adapter shell for:

- authenticated request gating
- session request special-casing
- response encoding branching
- `HistoryReadEvents` special handling

That split is reasonable at the wire boundary, but too much policy is still
owned there instead of by the shared runtime contract.

Current state:

- the Binary dispatcher now uses one typed visitor for authenticated runtime
  calls instead of a long per-request overload shell
- Binary session create / activate / close now live in that same dispatcher as
  explicit typed adapter methods that only fill in request-header/session-token
  data before calling the shared runtime
- `HistoryReadEvents` remains the main Binary-only response path because the
  Binary codec needs event-field-path context during encoding

Impact:

- parity fixes must be made in both the shared runtime and the Binary adapter
- it is harder to prove that Binary and WS implement the same service behavior

### 4. Service dispatch ownership was WS-biased

This gap is now closed: the canonical coroutine service dispatcher lives in
`common/opcua/opcua_service_handler.{h,cpp}` and the WS layer retains only
adapter-facing aliases plus transport/codec concerns.

### 5. Session behavior uses WS terminology for generic semantics

This gap is now mostly closed:

- the canonical implementation lives in
  `common/opcua/opcua_server_session_manager.{h,cpp}`,
  `common/opcua/opcua_server_session.{h,cpp}`, and
  `common/opcua/opcua_server_subscription.{h,cpp}`
- `common/opcua_ws/opcua_ws_session_manager.h`,
  `common/opcua_ws/opcua_ws_session.h`, and
  `common/opcua_ws/opcua_ws_subscription.h` now serve as adapter-facing alias
  wrappers only

Remaining work in this area is compatibility cleanup, not core ownership.

### 6. Cross-transport parity tests are missing

The repository has strong unit coverage inside each stack, but it does not yet
have a transport-neutral contract suite that asserts the same semantic outcome
for Binary and WS across the shared service set.

Impact:

- behavior can drift while both test suites still pass
- refactors remain higher-risk than they need to be

## Non-goals

This plan does not require:

- merging the Binary and WS wire codecs into one codec
- replacing WS UA-JSON framing with Binary framing
- replacing the Binary TCP / SecureChannel stack with the WS transport
- forcing the two transports to share handshake, origin policy, TLS policy, or
  connection lifecycle details

The target is one semantic runtime with two protocol adapters, not one protocol.

## Source of truth policy

When Binary and WS differ on OPC UA semantics, the source of truth should be
the OPC Foundation online references, not existing local behavior.

Use that check especially for:

- session lifecycle semantics
- `Publish` / keep-alive / republish behavior
- `TransferSubscriptions`
- monitored-item creation and initial-value rules
- `BrowseNext` continuation handling
- field-level wire-model mismatches between UA Binary and UA-JSON

If a discrepancy is transport-specific rather than semantic, keep it in the
adapter layer and document why.

## Gradual implementation plan

### Phase 1. Establish a canonical common protocol model

Goal:
Move the shared request/response and session/subscription data model out of the
WS-owned surface.

Implementation:

- introduce transport-neutral types under `common/opcua/` for:
  `CreateSession`, `ActivateSession`, `CloseSession`, service requests,
  subscription requests, monitored-item requests, publish messages, and service
  faults
- keep temporary `using` aliases in `opcua_ws` and `opcua` Binary headers so
  downstream code does not break all at once
- make `opcua_ws_message.h` and `opcua_binary_message.h` become adapter-facing
  wrappers over the canonical types instead of the origin of those types

Exit criteria:

- the canonical semantic model is owned by `common/opcua/`
- neither transport has to import the other's message definitions as the source
  of truth

### Phase 2. Make the runtime transport-neutral

Goal:
Change the shared runtime from "WS message router reused by Binary" into
"canonical OPC UA runtime used by both WS and Binary."

Implementation:

- give `OpcUaRuntime` a transport-neutral request/response contract
- keep connection attachment state generic
- move the request visitation currently in
  `common/opcua_ws/opcua_ws_runtime.cpp` onto the canonical request variants
- keep WS envelope handling outside the runtime
- keep Binary request-header decoding and response-header encoding outside the
  runtime

Exit criteria:

- Binary and WS both call the same runtime entrypoint with different adapter
  wrappers
- the runtime no longer exposes `OpcUaWsRequestMessage` or
  `OpcUaWsResponseMessage`

### Phase 3. Promote the service handler into the shared core

Goal:
Treat service dispatch as common OPC UA behavior rather than a WS helper.

Implementation:

- move or rename `opcua_ws_service_handler` into `common/opcua/`
- make it consume and produce canonical request/response types
- keep only transport-specific codec and framing logic in `common/opcua_ws/`

Exit criteria:

- the coroutine dispatch layer is owned by the shared OPC UA module
- endpoint behavior for `Read`, `Write`, `Browse`, `TranslateBrowsePaths`,
  `Call`, `HistoryRead`, and node-management operations is transport-neutral by
  construction

### Phase 4. Normalize shared session semantics

Goal:
Make the shared session and subscription core explicitly protocol-generic.

Implementation:

- rename WS-prefixed public session-facing types to canonical OPC UA names
- keep subscription ownership, publish arbitration, republish caching, and
  browse continuation state inside the shared session layer
- leave only transport delivery concerns outside that layer
- review semantic edge cases against the OPC UA reference before changing
  behavior

Exit criteria:

- `OpcUaSession` and `OpcUaSubscription` read as generic OPC UA server-session
  runtime objects
- WS-specific naming remains only in WS adapter and codec files

### Phase 5. Slim the Binary adapter down to wire translation

Goal:
Make Binary own Binary protocol details, not duplicated semantic policy.

Implementation:

- reduce `opcua_binary_service_dispatcher` to:
  decode Binary request, map to canonical request, invoke runtime, encode
  Binary response
- keep only true Binary-specific exceptions there, such as request-header
  handling and any response encoding requirements that do not exist in WS
- document any remaining Binary-only branches and why they cannot live in the
  shared runtime

Exit criteria:

- Binary adapter code becomes thin enough that semantic changes land in the
  shared core first
- Binary-specific code is explainable as transport or encoding adaptation only

Progress:

- the Binary request / response body aliases now point directly at the
  canonical `opcua::OpcUaRequestBody` / `opcua::OpcUaResponseBody` variants,
  leaving Binary-only type names as compatibility spellings over the shared
  schema
- `OpcUaBinaryRuntime` now owns Binary session-token lookup for session
  activate / close plus the trait-driven authenticated request-to-response
  mapping, leaving the dispatcher to decode requests, invoke the runtime, and
  encode responses
- the remaining Binary-only branch is the `HistoryReadEvents` response encoder,
  because it needs the request's decoded event-field path metadata
- the shared runtime now has one reusable contract-test suite that exercises
  the same semantic scenarios through the canonical runtime surface plus the
  Binary and WS adapter runtimes for read routing, detach/resume, transfer,
  close-session, and unauthenticated history-read behavior

### Phase 6. Add parity tests before larger behavior changes

Goal:
Make future unification work safe.

Implementation:

- add a transport-neutral contract suite for the shared runtime
- run the same semantic scenarios through Binary and WS adapters where practical
- cover at least:
  session create / activate / close
  read / write / browse
  browse continuation
  create / modify / delete subscription
  create / modify / delete monitored items
  publish / republish / transfer subscriptions
  history read and node management

Exit criteria:

- refactors can prove Binary and WS still agree semantically
- transport-specific tests focus on wire behavior, not shared service meaning

## Recommended execution order

1. Finish type ownership first.
2. Then switch the runtime contract.
3. Then move the service handler.
4. Then clean up session naming and semantics.
5. Then thin the Binary adapter.
6. Keep parity tests landing throughout, but require them before the larger
   runtime API cutover.

This order keeps the churn directional: ownership first, then API surface, then
adapter cleanup.

## Risks to manage

- accidental behavior drift while renaming WS-owned types into canonical names
- over-normalizing genuinely transport-specific differences
- breaking Binary response encoding while simplifying dispatch
- preserving current tested behavior even where that behavior may not match the
  OPC UA reference

The mitigation is to check semantic discrepancies against the OPC Foundation
reference before freezing the shared model, and to add parity tests before the
largest API moves.

## Expected end state

After this plan is complete:

- `common/opcua/` owns the canonical OPC UA server-runtime model
- `common/opcua_ws/` owns UA-JSON / WebSocket / WSS adaptation only
- Binary owns UA Binary / UACP / SecureChannel adaptation only
- behavior changes for server-side OPC UA services land once in the shared core
- transport modules become smaller, easier to review, and less likely to drift
