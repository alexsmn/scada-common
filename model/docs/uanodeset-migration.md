# Migrating the model nodesets to OPC UA UANodeSet2

## Why

`common/model/nodesets/*.xml` currently use a bespoke schema,
`<AddressSpace format="scada-node-state-v1">`. It is functional but proprietary:
no OPC UA tool can read or produce it, we hand-maintain a subset of the
namespace-0 base types, and there is no standard versioning/dependency metadata.

OPC UA's standard address-space interchange format is **UANodeSet** (a.k.a.
`*.NodeSet2.xml`), defined by `UANodeSet.xsd`
(`http://opcfoundation.org/UA/2011/03/UANodeSet.xsd`) and specified in **OPC UA
Part 6 (Mappings)** — https://reference.opcfoundation.org/Core/Part6/v105/docs/.
Models are shared as one `NodeSet2.xml` per namespace URI, carrying
`Model`/`RequiredModel` version + dependency metadata (**Part 5** —
https://reference.opcfoundation.org/Core/Part5/), distributed via the OPC
Foundation **UA-Nodeset** repo and **UA Cloud Library**, and consumed by every
OPC UA stack + modeling tool (UA-ModelCompiler, open62541, SiOME, UaModeler, …).

Adopting UANodeSet2 buys interoperability, the official NS0 nodeset for free,
standard versioning/discovery, and off-the-shelf codegen — at the cost of a new
parser and a converter.

## Invariants that must hold (do not regress)

1. **Numeric node-id values are frozen** — they are persisted in user config
   databases. `SCADA.115` → `ns=1;i=115` (file-local) / `ns=7;i=115` (server
   global): the numeric `115` never changes. `NS0.n` → `i=n`.
2. **SCADA stays at global namespace index 7.** UANodeSet uses a *file-local*
   namespace index remapped on load to the server's `NamespaceArray`; the server
   registration must keep the SCADA URI at index 7 so persisted ids resolve.
3. **Generated headers stay identical.** `ModelFrozenIds` + the generator
   round-trip diff remain the gate on any nodeset change.

## Helpful facts (verified in-tree)

- The custom format's numeric ids **already are** the OPC UA numeric ids for
  NS0: `NS0.40`=HasTypeDefinition, `NS0.37`=HasModellingRule, `NS0.45`=HasSubtype,
  `NS0.46`=HasProperty, `NS0.47`=HasComponent, `NS0.35`=Organizes, `NS0.58`=
  BaseObjectType, etc. So conversion is a **syntax transform, not a renumbering**.
- `opcuapp` (third_party) has **no** nodeset loader — the Phase 2 parser is
  bespoke, feeding the existing `common/address_space` `AddressSpace`.
- The loader (`common/address_space/address_space_xml.cpp`, `ReadNodeState` /
  `ApplyNodeStates`) builds a node's reference set from the **union** of:
  - `parent` + `parentReference`  → parent →(refType)→ node (inverse on node)
  - `supertype`                   → supertype →HasSubtype→ node (inverse on node)
  - `typeDefinition`              → node →HasTypeDefinition→ target (forward)
  - explicit `<Reference type= forward= target=>` children
  deduped by (refType, target, direction). `dataType` is a **Variable
  attribute**, not a reference. `<AttributeValue>` is the node's value;
  `<Property>` children carry instance property values.

## Format mapping

| `scada-node-state-v1` | UANodeSet2 |
|---|---|
| `<AddressSpace format="…">` | `<UANodeSet xmlns="…/UANodeSet.xsd">` + `<NamespaceUris>` + `<Aliases>` + `<Models>` |
| `class="ObjectType"` / `Variable` / `DataType` / `ReferenceType` / `Object` / `Method` / `VariableType` / `View` | `<UAObjectType>` / `<UAVariable>` / `<UADataType>` / `<UAReferenceType>` / `<UAObject>` / `<UAMethod>` / `<UAVariableType>` / `<UAView>` |
| `id="NS0.40"` | `NodeId="i=40"` |
| `id="SCADA.115"` | `NodeId="ns=1;i=115"` |
| `browseName="DeviceType"` | `BrowseName="1:DeviceType"` |
| `displayName="…"` | `<DisplayName>…</DisplayName>` |
| `symbolicName="…"` (ours) | `SymbolicName="…"` (**the standard attribute**, used by codegen) |
| `codeNs="…"` (ours) | no standard equivalent → codegen sidecar (see Generator) |
| `parent`+`parentReference`, `supertype`, `typeDefinition`, `<Reference …>` | `<References><Reference ReferenceType="<alias>" IsForward="true|false">ns=…;i=…</Reference>…</References>` (deduped union) |
| `dataType="NS0.1"` | `<UAVariable DataType="Boolean">` (attribute) |
| `<AttributeValue type="Bool">true</AttributeValue>` | `<Value><uax:Boolean>true</uax:Boolean></Value>` |
| — | `<Aliases>`: numeric ref-type/datatype ids → names (HasSubtype=i=45, …) |
| — | `<Models><Model ModelUri Version PublicationDate><RequiredModel …/></Model></Models>` |

Reference emission convention (matches published nodesets): `HasTypeDefinition`
forward; the inverse hierarchical reference to the parent (`HasComponent` /
`HasProperty` / `Organizes` / `HasSubtype`) with `IsForward="false"`; other owned
references forward.

## Phased plan

### Phase 0 — Decisions (no code)
- **Namespace URIs**: add a `uri` column to `namespaces.csv`. NS0 =
  `http://opcfoundation.org/UA/`; SCADA = a chosen vendor URI (e.g.
  `http://telecontrol.example/SCADA/`). Instance namespaces (USER, config-DB, …)
  only need URIs if they are also modeled.
- **NS0 scope**: import the official `Opc.Ua.NodeSet2.xml` (complete, large) vs.
  keep a **curated UANodeSet subset** of the types actually referenced.
  Recommended: curated subset first.
- **Coexistence**: keep both loaders during Phases 2–4; cut over at Phase 4.

### Phase 1 — One-shot converter + validated sample  ← *this change*
`gen/convert_to_uanodeset.py`: transform the custom XML → UANodeSet2 per the
mapping above (per-class elements, `ns=;i=` ids with file-local namespace,
deduped reference union, aliases, `uax:` value encoding, `Model` metadata).
Emit `Scada.NodeSet2.xml` (the SCADA-namespace nodes) for review and validate it
against `UANodeSet.xsd`. Originals stay in place.

### Phase 2 — UANodeSet2 loader (DONE)
`LoadUANodeSetXml` / `LoadStaticUANodeSet` in `common/address_space/` parse
UANodeSet2 into the **same** `NodeState` vector the custom parser builds and hand
it to the shared `ApplyNodeStates`, so materialization is identical. Each node's
`<References>` are reversed back into the `NodeState` hierarchy fields
(`HasSubtype`→supertype, `HasComponent`/`HasProperty`/`Organizes` inverse→parent,
`HasTypeDefinition` forward→type definition); everything else stays in
`references`. Also added `LoadStaticNodesets`, which auto-detects each file's
format from its root element so custom and UANodeSet2 files can be loaded
together during the transition.

**A/B test** (`address_space_uanodeset_unittest.cpp`, `UANodeSetLoader.
MatchesCustomFormat`): loads the model both ways — custom (all 11 files) vs
`opcua_base.xml` (custom, ns0) + `Scada.Full.NodeSet2.xml` (UANodeSet) — and
asserts the two address spaces are node-for-node identical (294 nodes: class,
browse name, hierarchy, references, values). Passes.

Two behaviors the converter must preserve for a faithful drop-in (both learned
here, both handled):
- **Load order matters.** The factory instantiates a type's properties as nested
  instance nodes only if the type is already built, so a type must precede its
  instances. The converter emits files in the canonical `kScadaStaticNodesetFiles`
  order (scada_core before history, …), not glob order.
- **Browse-name namespace is 0.** The custom format defaults a browse name to
  namespace 0 (no `browseNamespace` attribute), so SCADA nodes carry ns0 browse
  names. The converter preserves that (Phase 5 could normalize browse names to
  the node's namespace, but that changes behavior).

### Phase 3 — Generator update (DONE)
`generate_model_headers.py` is now format-agnostic: `read_symbolic_nodes`
detects each nodeset file's root and reads `symbolicName`/`SymbolicName` +
numeric id from either format. The C++ sub-domain grouping (`devices`,
`data_items`, …) is not an OPC UA concept, so `codeNs` moved out of the XML into
the **`nodesets/code_domains.csv`** sidecar (symbolic-name → domain), read by the
generator for both formats. `namespaces.csv` / `extra_node_ids.csv` unchanged.

Proven byte-identical two ways: the generator's output from the current custom
nodesets equals its output from the converted `Scada.Full.NodeSet2.xml`
(`diff -rq` → identical), and the build (still on the custom nodesets) keeps
`ModelFrozenIds` green. So the generator is ready for the Phase 4 cut-over — only
the input files change, not its output.

### Phase 4 — Cut over (DONE)
The SCADA model is now `nodesets/Scada.NodeSet2.xml` (standard UANodeSet2); the
10 custom SCADA files are removed. `opcua_base.xml` stays in the repo-owned
format for now (the ns0 base subset; Phase 5 can swap in the official OPC UA
nodeset). `kScadaStaticNodesetFiles` is `{opcua_base.xml, Scada.NodeSet2.xml}`.
`LoadStaticAddressSpace` auto-detects each file's format, so every runtime caller
loads the mixed set unchanged — no caller churn. The generator reads the new SoT
(opcua_base contributes no constants; `Scada.NodeSet2.xml` supplies them via
`SymbolicName` + the sidecar).

The Phase 2 A/B test is replaced by `ScadaAddressSpace.MatchesGolden`: a committed
golden dump (`common/address_space/scada_address_space_golden.txt`) captured from
the original scada-node-state-v1 model, against which the loaded address space is
checked node-for-node. Verified: `ModelFrozenIds`, `ScadaAddressSpace.
MatchesGolden`, `StaticNodesets.ResolveAcrossFiles`, and the server-module
`CoreModuleTest` all pass on the new SoT. The one-shot migration tools
(`convert_to_uanodeset.py`, `seed_from_headers.py`) are retired; git history and
this doc preserve the mapping.

### Phase 5 — Standardize & publish (optional, high value)
Add `Model` metadata; publish via the existing **NamespaceMetadata** machinery
(`Server/Namespaces`, Part 5) for runtime discovery; optionally swap the curated
NS0 for the official nodeset; consider off-the-shelf codegen.

## Risks / open decisions
- **Reference-type / datatype aliases**: need the numeric-id → standard-name
  table (mostly the NS0 set above + a few vendor ref types like `SCADA.311`
  HasPropertyCategory, `SCADA.297` Creates). One-time map.
- **Value encoding fidelity**: few default values exist, but `uax:` variant
  encode/decode must be exact.
- **`codeNs` has no standard home** — accept a codegen-only sidecar.
- **NS0 scope** — full official nodeset enlarges the runtime address space; a
  curated subset is the pragmatic first step.
- **Parser vs library** — no in-tree library fits; bespoke-feeding-the-existing
  model is lowest risk.

## Effort
Phase 1 converter and Phase 3 generator update are small; **Phase 2 loader is the
bulk** (a few hundred lines + value coding). Phases 4–5 are wiring/config.
