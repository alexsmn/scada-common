#!/usr/bin/env python3
"""Build-time generator: emit the model's namespace + node-id C++ headers from
the nodeset XML (the single source of truth) plus the small `namespaces.csv` /
`extra_node_ids.csv` manifests.

Inputs (under --nodesets):
  * `*.xml`            — static address-space nodes. Every `<Node>` that carries
                         a `symbolicName` attribute yields a `scada::<domain>::id`
                         constant; `codeNs` overrides the file-default domain.
  * `namespaces.csv`   — index, C++ const name, config-DB short name.
  * `extra_node_ids.csv` — constants with no static node (runtime method ids,
                         server metrics, reserved ids) and all `numeric_id`
                         constants.

Outputs (under --out, one dir):
  namespaces.h, namespaces.cpp, and `<domain>_node_ids.h` per domain.

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
                   ['#include "scada/node_id.h"', '#include "model/namespaces.h"',
                    '#include "model/model_compat.h"']),
    "devices": ("devices_node_ids.h", "scada::devices",
                ['#include "scada/node_id.h"', '#include "model/namespaces.h"',
                 '#include "model/model_compat.h"']),
    "history": ("history_node_ids.h", "scada::history",
                ['#include "model/namespaces.h"', '#include "scada/node_id.h"',
                 '#include "model/model_compat.h"']),
    "security": ("security_node_ids.h", "scada::security",
                 ['#include "model/namespaces.h"', '#include "scada/node_id.h"',
                  '#include "scada/standard_node_ids.h"',
                  '#include "model/model_compat.h"']),
    "filesystem": ("filesystem_node_ids.h", "scada::filesystem",
                   ['#include "scada/node_id.h"', '#include "model/namespaces.h"',
                    '#include "model/model_compat.h"']),
    "opc": ("opc_node_ids.h", "scada::opc",
            ['#include "scada/node_id.h"', '#include "model/namespaces.h"',
             '#include "model/model_compat.h"']),
}
GEN_BANNER = ("// GENERATED FILE - DO NOT EDIT.\n"
              "// Produced by common/model/gen/generate_model_headers.py from the\n"
              "// nodeset XML source of truth. Edit the nodesets (symbolicName/codeNs)\n"
              "// or the CSV manifests instead, then rebuild.\n")


def load_namespaces(path):
    rows = []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            rows.append((int(r["index"]), r["const_name"], r["db_name"]))
    return rows


def ns_const_by_index(ns_rows):
    return {i: c for i, c, _d in ns_rows if c}


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
    for i, const, _db in ns_rows:
        if const:
            const_lines.append(f"constexpr scada::NamespaceIndex {const} = {i};")
        else:
            const_lines.append(f"// index {i} has no C++ constant (config-DB name only)")
    body = "\n".join(const_lines)
    return f"""{GEN_BANNER}#pragma once

#include "scada/basic_types.h"

#include <string_view>

#include "model/model_compat.h"
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
    names = ",\n    ".join(f'"{db}"' for _i, _c, db in ns_rows)
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

static_assert(std::size(kNamespaceNames) == NamespaceIndexes::END);

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
    ns_const_index = {i: c for i, c, _d in ns_rows if c}
    ns_const_index_all = {i: (c if c else str(i)) for i, c, _d in ns_rows}

    consts = collect_constants(args.nodesets)

    os.makedirs(args.out, exist_ok=True)
    written = []
    with open(os.path.join(args.out, "namespaces.h"), "w") as f:
        f.write(emit_namespaces_h(ns_rows))
    with open(os.path.join(args.out, "namespaces.cpp"), "w") as f:
        f.write(emit_namespaces_cpp(ns_rows))
    written += ["namespaces.h", "namespaces.cpp"]
    for domain in DOMAINS:
        fname, text = emit_domain_header(domain, consts[domain], ns_const_index_all)
        with open(os.path.join(args.out, fname), "w") as f:
            f.write(text)
        written.append(fname)
    print("generated:", ", ".join(written))


if __name__ == "__main__":
    main()
