#!/usr/bin/env python3
"""Build-time generator: emit the model's namespace + node-id C++ headers from
the nodeset XML (the single source of truth) plus the small `namespaces.csv` /
`extra_node_ids.csv` manifests.

Inputs (under --nodesets):
  * `*.xml`            — static address-space nodes. Every `<Node>` that carries
                         a `symbolicName` attribute yields a `scada::<domain>::id`
                         constant; `codeNs` overrides the file-default domain.
  * `namespaces.csv`   — index, C++ const name, config-DB short name, plus the
                         optional config-table registry columns: `row_type`
                         (symbolic name of the namespace's row type),
                         `config_group` (which config group serves the table)
                         and `group_kind` (`device` = hosted by the generic
                         device-config module on the config server and by a
                         driver tier on the edge; `dedicated` = hosted by that
                         group's dedicated framework module — data_items,
                         history, security). All three set or all empty per
                         row; a group's kind must be consistent across rows.
  * `extra_node_ids.csv` — constants with no static node (runtime method ids,
                         server metrics, reserved ids) and all `numeric_id`
                         constants.

Outputs (under --out, one dir):
  namespaces.h, namespaces.cpp, config_tables.h, and `<domain>_node_ids.h` per
  domain.

The generator is deterministic (constants sorted by value then name) so repeated
builds produce identical output. Node numeric ids are read-only: a `symbolicName`
renames the C++ handle but never changes the persisted id.
"""
import argparse
import csv
import glob
import os
import re

NS_PREFIX = {"NS0": 0, "SCADA": 7}
DOMAIN_BY_XML = {
    "scada_core.xml": "scada", "data_items.xml": "data_items",
    "devices.xml": "devices", "devices_modbus.xml": "devices",
    "devices_iec60870.xml": "devices", "devices_iec61850.xml": "devices",
    "history.xml": "history", "security.xml": "security",
    "filesystem.xml": "filesystem", "opc.xml": "opc", "opcua_base.xml": "ns0",
}
# domain -> (header filename, C++ namespace, include lines)
DOMAINS = {
    "scada": ("scada_node_ids.h", "scada",
              ['#include "scada/standard_node_ids.h"', '#include "model/namespaces.h"']),
    "data_items": ("data_items_node_ids.h", "scada::data_items",
                   ['#include "scada/node_id.h"', '#include "model/namespaces.h"']),
    "devices": ("devices_node_ids.h", "scada::devices",
                ['#include "scada/node_id.h"', '#include "model/namespaces.h"']),
    "history": ("history_node_ids.h", "scada::history",
                ['#include "model/namespaces.h"', '#include "scada/node_id.h"']),
    "security": ("security_node_ids.h", "scada::security",
                 ['#include "model/namespaces.h"', '#include "scada/node_id.h"',
                  '#include "scada/standard_node_ids.h"']),
    "filesystem": ("filesystem_node_ids.h", "scada::filesystem",
                   ['#include "scada/node_id.h"', '#include "model/namespaces.h"']),
    "opc": ("opc_node_ids.h", "scada::opc",
            ['#include "scada/node_id.h"', '#include "model/namespaces.h"']),
}
GEN_BANNER = ("// GENERATED FILE - DO NOT EDIT.\n"
              "// Produced by common/model/gen/generate_model_headers.py from the\n"
              "// nodeset XML source of truth. Edit the nodesets (symbolicName/codeNs)\n"
              "// or the CSV manifests instead, then rebuild.\n")


def load_namespaces(path):
    rows = []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            rows.append((int(r["index"]), r["const_name"], r["db_name"],
                         r.get("row_type", "") or "",
                         r.get("config_group", "") or "",
                         r.get("group_kind", "") or ""))
    return rows


def read_code_domains(path):
    """symbolic_name -> C++ domain. The domain (which header a constant lands in)
    is not an OPC UA concept, so it lives in this sidecar rather than in the
    nodesets. See common/model/docs/uanodeset-migration.md."""
    domains = {}
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            domains[r["symbolic_name"]] = r["domain"]
    return domains


def read_symbolic_nodes(path):
    """Yield (symbolic_name, ns_index, numeric_id) for every annotated node,
    auto-detecting the file format (repo-owned scada-node-state-v1 or standard
    UANodeSet2). Numeric ids are identical in both; only the syntax differs."""
    text = open(path, encoding="utf-8").read()
    if re.search(r"<UANodeSet\b", text):
        for m in re.finditer(r"<UA\w+\s+([^>]*?)/?>", text):
            attrs = dict(re.findall(r'(\w+)="([^"]*)"', m.group(1)))
            sym = attrs.get("SymbolicName")
            idm = re.match(r"(?:ns=(\d+);)?i=(\d+)", attrs.get("NodeId", ""))
            if not sym or not idm:
                continue
            ns_index = 7 if (idm.group(1) or "0") == "1" else 0  # local ns 1 == SCADA
            yield sym, ns_index, int(idm.group(2))
    else:
        for m in re.finditer(r"<Node\s+([^>]*?)/?>", text):
            attrs = dict(re.findall(r'(\w+)="([^"]*)"', m.group(1)))
            sym = attrs.get("symbolicName")
            idm = re.match(r"([A-Z0-9]+)\.(\d+)", attrs.get("id", ""))
            if not sym or not idm:
                continue
            yield sym, NS_PREFIX.get(idm.group(1), -1), int(idm.group(2))


def collect_constants(nodesets_dir):
    """Return {domain: {'id': {name: (value, ns_index)}, 'numeric_id': {name: value}}}."""
    out = {d: {"id": {}, "numeric_id": {}} for d in DOMAINS}
    domains = read_code_domains(os.path.join(nodesets_dir, "code_domains.csv"))
    # From nodes (either format); the sidecar decides the C++ domain.
    for path in sorted(glob.glob(os.path.join(nodesets_dir, "*.xml"))):
        for sym, ns_index, value in read_symbolic_nodes(path):
            domain = domains.get(sym)
            if not domain or domain == "ns0":
                continue  # ns0 base nodes carry no SCADA C++ constant
            out[domain]["id"][sym] = (value, ns_index)
    # From the supplement CSV.
    csv_path = os.path.join(nodesets_dir, "extra_node_ids.csv")
    with open(csv_path, newline="") as f:
        for r in csv.DictReader(f):
            domain, kind, name, value = r["domain"], r["kind"], r["name"], int(r["value"])
            if kind == "numeric_id":
                out[domain]["numeric_id"][name] = value
            else:
                # ns index from ns_const via caller; SCADA is the only case here.
                out[domain]["id"][name] = (value, NS_PREFIX.get("SCADA", 7))
    return out


def emit_namespaces_h(ns_rows):
    const_lines = []
    end = len(ns_rows)
    for i, const, _db, _rt, _cg, _gk in ns_rows:
        if const:
            const_lines.append(f"constexpr scada::NamespaceIndex {const} = {i};")
        else:
            const_lines.append(f"// index {i} has no C++ constant (config-DB name only)")
    body = "\n".join(const_lines)
    return f"""{GEN_BANNER}#pragma once

#include "scada/basic_types.h"

#include <string_view>

namespace scada::NamespaceIndexes {{

{body}

constexpr scada::NamespaceIndex END = {end};

}}  // namespace scada::NamespaceIndexes

namespace scada {{
// Short name of a namespace index (its config-DB table name), or empty if out
// of range.
std::string_view GetNamespaceName(scada::NamespaceIndex namespace_index);
// Namespace index for a short name (case-insensitive) or numeric "N"/"TN"
// form; -1 if unknown.
int FindNamespaceIndexByName(std::string_view name);
}}  // namespace scada

// Transitional compatibility shim: expose the historically global-scope names
// until all callers migrate to `scada::`.
using scada::FindNamespaceIndexByName;  // NOLINT(build/namespaces) transitional
using scada::GetNamespaceName;          // NOLINT(build/namespaces) transitional
"""


def emit_namespaces_cpp(ns_rows):
    names = ",\n    ".join(f'"{db}"' for _i, _c, db, _rt, _cg, _gk in ns_rows)
    return f"""{GEN_BANNER}#include "model/namespaces.h"

#include <boost/algorithm/string/predicate.hpp>

#include <cstring>

#if defined(SCADA_USE_CORE_MODULE)
import scada.core;
#else
#include "base/format.h"
#endif

namespace {{

// WARNING: These names are used as names for tables in the configuration DB and
// mustn't be modified.
constexpr std::string_view kNamespaceNames[] = {{
    {names},
}};

static_assert(std::size(kNamespaceNames) == scada::NamespaceIndexes::END);

}}  // namespace

namespace scada {{

std::string_view GetNamespaceName(scada::NamespaceIndex namespace_index) {{
  if (namespace_index >= 0 && namespace_index < NamespaceIndexes::END)
    return kNamespaceNames[namespace_index];
  else
    return {{}};
}}

int FindNamespaceIndexByName(std::string_view name) {{
  if (name.empty())
    return -1;

  int namespace_index = -1;
  if (name[0] == 'T' && Parse(name.substr(1), namespace_index)) {{
    return namespace_index;
  }}

  if (Parse(name, namespace_index))
    return namespace_index;

  for (scada::NamespaceIndex i = 0; i != NamespaceIndexes::END; ++i) {{
    if (boost::iequals(GetNamespaceName(i), name)) {{
      return i;
    }}
  }}

  return -1;
}}

}}  // namespace scada
"""


def resolve_row_type(name, consts):
    """Resolve a row_type symbolic name to its (value, ns_index) across all
    domains. Hard error on unknown or ambiguous names so a nodeset rename can't
    silently drop a table from the registry."""
    hits = [(domain, data["id"][name])
            for domain, data in consts.items() if name in data["id"]]
    if not hits:
        raise SystemExit(
            f"namespaces.csv row_type '{name}' does not resolve to any "
            f"symbolicName in the nodesets")
    if len(hits) > 1:
        raise SystemExit(
            f"namespaces.csv row_type '{name}' is ambiguous across domains: "
            f"{sorted(d for d, _ in hits)}")
    return hits[0][1]


def emit_config_tables_h(ns_rows, consts, ns_const_index_all):
    entries = []
    groups = []  # (name, kind), first-appearance order
    module_entries = []       # (namespace_index, config_group) for module rows
    module_groups = []        # module group names, first-appearance order
    seen_row_types = set()
    for i, const, db, row_type, config_group, group_kind in ns_rows:
        # config_group and group_kind travel together; row_type is present only
        # for config-DB-backed tables ("device"/"dedicated"). A "module" group
        # (filesystem/opc/vidicon) owns a namespace with no config table, so it
        # carries config_group + group_kind but no row_type.
        if bool(config_group) != bool(group_kind):
            raise SystemExit(
                f"namespaces.csv index {i}: config_group and group_kind must "
                f"both be set or both be empty (got config_group="
                f"'{config_group}', group_kind='{group_kind}')")
        if group_kind == "module":
            if row_type:
                raise SystemExit(
                    f"namespaces.csv index {i}: a 'module' group owns a "
                    f"namespace with no config table, so row_type must be "
                    f"empty (got row_type='{row_type}')")
            module_entries.append(
                (ns_const_index_all[i], config_group))
            if config_group not in module_groups:
                module_groups.append(config_group)
            continue
        if bool(row_type) != bool(config_group):
            raise SystemExit(
                f"namespaces.csv index {i}: a 'device'/'dedicated' group is a "
                f"config table, so row_type, config_group and group_kind must "
                f"all be set or all be empty (got row_type='{row_type}', "
                f"config_group='{config_group}', group_kind='{group_kind}')")
        if not row_type:
            continue
        if group_kind not in ("device", "dedicated"):
            raise SystemExit(
                f"namespaces.csv index {i}: group_kind must be 'device', "
                f"'dedicated' or 'module', got '{group_kind}'")
        if row_type in seen_row_types:
            raise SystemExit(
                f"namespaces.csv index {i}: row_type '{row_type}' already "
                f"maps to another namespace — one row type per table")
        seen_row_types.add(row_type)
        for name, kind in groups:
            if name == config_group and kind != group_kind:
                raise SystemExit(
                    f"namespaces.csv index {i}: config_group "
                    f"'{config_group}' has inconsistent group_kind "
                    f"('{kind}' vs '{group_kind}')")
        value, type_ns = resolve_row_type(row_type, consts)
        type_ns_const = ns_const_index_all.get(type_ns, str(type_ns))
        table_ns_const = ns_const_index_all[i]
        entries.append(
            f"    // {row_type} rows live in the {db} table.\n"
            f"    {{{{{value}, NamespaceIndexes::{type_ns_const}}},\n"
            f"     NamespaceIndexes::{table_ns_const},\n"
            f'     "{config_group}",\n'
            f'     "{group_kind}"}},')
        if config_group not in [g for g, _k in groups]:
            groups.append((config_group, group_kind))
    entry_body = "\n".join(entries)
    group_body = ", ".join(f'"{g}"' for g, _k in groups)
    device_group_body = ", ".join(
        f'"{g}"' for g, k in groups if k == "device")
    # A module namespace may have no NamespaceIndexes:: constant (the generator
    # emits constants only for names referenced from C++); fall back to the raw
    # numeric index in that case.
    def ns_index_expr(ns_const):
        return ns_const if ns_const.isdigit() else f"NamespaceIndexes::{ns_const}"
    module_entry_body = "\n".join(
        f'    {{{ns_index_expr(ns_const)}, "{group}"}},'
        for ns_const, group in module_entries)
    module_group_body = ", ".join(f'"{g}"' for g in module_groups)
    return f"""{GEN_BANNER}#pragma once

#include "model/namespaces.h"
#include "scada/node_id.h"

#include <span>
#include <string_view>

namespace scada::model {{

// A model-owned configuration table: the table's row type, the namespace
// (config-DB table) its rows live in, the config group that serves it, and
// the group's kind — "device" groups are hosted by the generic device-config
// module on the config server (and by their driver tier on the edge), while
// "dedicated" groups (data_items, history, security) are hosted by their
// dedicated framework module. Generated from the
// `row_type`/`config_group`/`group_kind` columns of
// common/model/nodesets/namespaces.csv — the single source of truth for the
// type<->namespace association shared by the tiers and the config server.
struct ConfigTableEntry {{
  scada::NodeId type_definition_id;
  scada::NamespaceIndex namespace_index;
  std::string_view config_group;
  std::string_view group_kind;
}};

inline constexpr std::string_view kDeviceGroupKind = "device";

inline constexpr ConfigTableEntry kConfigTables[] = {{
{entry_body}
}};

// Distinct config groups, in registry (namespace-index) order.
inline constexpr std::string_view kConfigTableGroups[] = {{{group_body}}};

// Distinct device-kind config groups, in registry order.
inline constexpr std::string_view kDeviceConfigTableGroups[] = {{{device_group_body}}};

// All model-registered configuration tables, in registry order.
constexpr std::span<const ConfigTableEntry> GetConfigTables() {{
  return kConfigTables;
}}

// The distinct config groups of GetConfigTables().
constexpr std::span<const std::string_view> GetConfigTableGroups() {{
  return kConfigTableGroups;
}}

// The distinct device-kind config groups of GetConfigTables().
constexpr std::span<const std::string_view> GetDeviceConfigTableGroups() {{
  return kDeviceConfigTableGroups;
}}

inline constexpr std::string_view kModuleGroupKind = "module";

// A namespace owned exclusively by one module's tier — the filesystem, opc and
// vidicon tiers each mint their instance nodes in their own namespace and host
// no config table there. Unlike a "device" group this has no ConfigTableEntry
// (no row type). A tier that does not serve the group drops the namespace from
// its published NamespaceArray, so an aggregating proxy routes it to the owning
// tier without a config claim (ADR 0003). The types these instances use live in
// the shared SCADA namespace, which every tier keeps.
struct ModuleNamespaceEntry {{
  scada::NamespaceIndex namespace_index;
  std::string_view config_group;
}};

inline constexpr ModuleNamespaceEntry kModuleNamespaces[] = {{
{module_entry_body}
}};

// The module-owned namespaces (filesystem/opc/vidicon instance namespaces).
constexpr std::span<const ModuleNamespaceEntry> GetModuleNamespaces() {{
  return kModuleNamespaces;
}}

// Distinct module groups, in registry (namespace-index) order.
inline constexpr std::string_view kModuleNamespaceGroups[] = {{{module_group_body}}};

// The distinct module groups that own a namespace (filesystem/opc/vidicon).
constexpr std::span<const std::string_view> GetModuleNamespaceGroups() {{
  return kModuleNamespaceGroups;
}}

}}  // namespace scada::model
"""


def emit_domain_header(domain, data, ns_const_index):
    fname, cpp_ns, includes = DOMAINS[domain]
    parts = [GEN_BANNER, "#pragma once", ""]
    parts += includes
    parts += ["", f"namespace {cpp_ns} {{", ""]
    numeric = data["numeric_id"]
    if numeric:
        parts.append("namespace numeric_id {")
        parts.append("")
        for name in sorted(numeric, key=lambda n: (numeric[n], n)):
            parts.append(f"constexpr scada::NumericId {name} = {numeric[name]};")
        parts.append("")
        parts.append("}  // namespace numeric_id")
        parts.append("")
    ids = data["id"]
    parts.append("namespace id {")
    parts.append("")
    for name in sorted(ids, key=lambda n: (ids[n][0], n)):
        value, ns_index = ids[name]
        ns_const = ns_const_index.get(ns_index, str(ns_index))
        parts.append(
            f"constexpr scada::NodeId {name}{{{value}, NamespaceIndexes::{ns_const}}};")
    parts.append("")
    parts.append("}  // namespace id")
    parts.append("")
    parts.append(f"}}  // namespace {cpp_ns}")
    parts.append("")
    return fname, "\n".join(parts)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--nodesets", required=True, help="dir with *.xml + CSVs")
    ap.add_argument("--out", required=True, help="output dir for generated headers")
    args = ap.parse_args()

    ns_rows = load_namespaces(os.path.join(args.nodesets, "namespaces.csv"))
    ns_const_index = {i: c for i, c, _d, _rt, _cg, _gk in ns_rows if c}
    ns_const_index_all = {
        i: (c if c else str(i)) for i, c, _d, _rt, _cg, _gk in ns_rows}

    consts = collect_constants(args.nodesets)

    os.makedirs(args.out, exist_ok=True)
    written = []
    with open(os.path.join(args.out, "namespaces.h"), "w") as f:
        f.write(emit_namespaces_h(ns_rows))
    with open(os.path.join(args.out, "namespaces.cpp"), "w") as f:
        f.write(emit_namespaces_cpp(ns_rows))
    with open(os.path.join(args.out, "config_tables.h"), "w") as f:
        f.write(emit_config_tables_h(ns_rows, consts, ns_const_index_all))
    written += ["namespaces.h", "namespaces.cpp", "config_tables.h"]
    for domain in DOMAINS:
        fname, text = emit_domain_header(domain, consts[domain], ns_const_index_all)
        with open(os.path.join(args.out, fname), "w") as f:
            f.write(text)
        written.append(fname)
    print("generated:", ", ".join(written))


if __name__ == "__main__":
    main()
