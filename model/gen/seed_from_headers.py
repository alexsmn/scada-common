#!/usr/bin/env python3
"""One-shot seeding tool: derive the generator's source-of-truth annotations
from the current hand-written `core/model/*_node_ids.h` headers.

It produces, from the existing headers + nodeset XML:
  * `symbolicName`/`codeNs` attributes injected into the nodeset `<Node>`s
    (written in place into the XML files under --nodesets), for every node that
    backs a C++ node-id constant.
  * `extra_node_ids.csv` — constants that have no backing static node (runtime
    method ids, server metrics, reserved bookkeeping ids, and id-collision
    aliases).
  * `namespaces.csv` — the namespace index manifest (index, C++ const name,
    config-DB short name), reconciled from `namespaces.h`/`namespaces.cpp`.

This is not part of the build; it exists so the initial annotations are
mechanically derived (and therefore faithful) rather than hand-transcribed.
After seeding, `generate_model_headers.py` is the forward build step.
"""
import argparse
import csv
import glob
import os
import re

DOMAIN_BY_HEADER = {
    "scada_node_ids.h": "scada",
    "data_items_node_ids.h": "data_items",
    "devices_node_ids.h": "devices",
    "history_node_ids.h": "history",
    "security_node_ids.h": "security",
    "filesystem_node_ids.h": "filesystem",
    "opc_node_ids.h": "opc",
}
# Which XML files default to which C++ domain (for the collision tie-break and
# to minimize explicit codeNs overrides).
DOMAIN_BY_XML = {
    "scada_core.xml": "scada",
    "data_items.xml": "data_items",
    "devices.xml": "devices",
    "devices_modbus.xml": "devices",
    "devices_iec60870.xml": "devices",
    "devices_iec61850.xml": "devices",
    "history.xml": "history",
    "security.xml": "security",
    "filesystem.xml": "filesystem",
    "opc.xml": "opc",
    "opcua_base.xml": "ns0",
}
NS_PREFIX = {"NS0": 0, "SCADA": 7}


def load_ns_indices(model_dir):
    nsmap = {}
    for line in open(os.path.join(model_dir, "namespaces.h")):
        m = re.match(r"\s*constexpr scada::NamespaceIndex\s+([A-Z0-9_]+)\s*=\s*(\d+)\s*;", line)
        if m:
            nsmap[m.group(1)] = int(m.group(2))
    return nsmap


def parse_header(path, nsmap):
    """Yield (name, kind, value, ns_index, ns_const) for each constant.

    Multi-line-tolerant (some definitions wrap the initializer across lines) and
    comment-stripped. A constant whose value is not a plain integer or a
    `numeric_id::` reference, or whose namespace is not SCADA (e.g. the `RoleSet`
    alias to an NS0 standard node), is reported with kind "special" so the caller
    keeps it hand-written rather than trying to generate it."""
    text = re.sub(r"//[^\n]*", "", open(path).read())
    numeric_local = {}
    out = []
    for m in re.finditer(r"const scada::NumericId\s+(\w+)\s*=\s*(\d+)\s*;", text):
        numeric_local[m.group(1)] = int(m.group(2))
        out.append((m.group(1), "numeric_id", int(m.group(2)), None, None))
    for m in re.finditer(
            r"const scada::NodeId\s+(\w+)\s*\{\s*([^,}]+?)\s*,\s*([A-Za-z0-9_:]+)\s*\}", text):
        name = m.group(1)
        val = m.group(2).strip()
        ns_const = m.group(3).split("::")[-1]
        is_numeric = val.isdigit() or val.startswith("numeric_id::")
        if not is_numeric or ns_const != "SCADA":
            out.append((name, "special", val, nsmap.get(ns_const), ns_const))
            continue
        valn = numeric_local.get(val.split("::")[-1]) if val.startswith("numeric_id::") else int(val)
        out.append((name, "id", valn, nsmap.get(ns_const), ns_const))
    return out


def load_xml_nodes(nodesets_dir):
    """Return {(ns_index, id): {'file','browseName','line','attrs'}} for <Node>s."""
    nodes = {}
    for path in sorted(glob.glob(os.path.join(nodesets_dir, "*.xml"))):
        fname = os.path.basename(path)
        for m in re.finditer(r"<Node ([^>]*?)/?>", open(path).read()):
            attrs = dict(re.findall(r'(\w+)="([^"]*)"', m.group(1)))
            idm = re.match(r"([A-Z0-9]+)\.(\d+)", attrs.get("id", ""))
            if not idm:
                continue
            key = (NS_PREFIX.get(idm.group(1), -1), int(idm.group(2)))
            nodes[key] = {"file": fname, "browseName": attrs.get("browseName", "")}
    return nodes


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--headers", default="core/model", help="dir with *_node_ids.h")
    ap.add_argument("--nodesets", default="server/base/nodesets",
                    help="dir with nodeset XML to annotate in place")
    ap.add_argument("--out", default="common/model/nodesets",
                    help="dir to write extra_node_ids.csv / namespaces.csv")
    ap.add_argument("--apply-xml", action="store_true",
                    help="inject symbolicName/codeNs into the XML files")
    args = ap.parse_args()

    nsmap = load_ns_indices(args.headers)
    xml_nodes = load_xml_nodes(args.nodesets)

    # Gather all id constants with their domain.
    id_consts = []   # (domain, name, value, ns_index, ns_const)
    numeric_consts = []  # (domain, name, value)
    specials = []  # (domain, name, value_expr, ns_const) -> stay hand-written
    for header in sorted(glob.glob(os.path.join(args.headers, "*_node_ids.h"))):
        domain = DOMAIN_BY_HEADER.get(os.path.basename(header))
        if not domain:
            continue
        for name, kind, value, ns_index, ns_const in parse_header(header, nsmap):
            if kind == "id":
                id_consts.append((domain, name, value, ns_index, ns_const))
            elif kind == "numeric_id":
                numeric_consts.append((domain, name, value))
            else:  # "special": non-numeric value / non-SCADA ns -> hand-written
                specials.append((domain, name, value, ns_const))

    # Assign each node its symbolicName. On collision (same ns.id backing >1
    # constant), the constant whose domain == the node's file-default domain
    # wins the node; the rest become supplements.
    claimed = {}   # (ns,val) -> (domain, name, ns_const)
    supplements = []   # (domain, sub, name, value, ns_const)
    by_key = {}
    for c in id_consts:
        by_key.setdefault((c[3], c[2]), []).append(c)

    for key, cands in by_key.items():
        node = xml_nodes.get(key)
        if node is None:
            for domain, name, value, ns_index, ns_const in cands:
                supplements.append((domain, "id", name, value, ns_const))
            continue
        file_domain = DOMAIN_BY_XML.get(node["file"], "scada")
        winner = None
        for c in cands:
            if c[0] == file_domain:
                winner = c
                break
        if winner is None:
            winner = cands[0]
        claimed[key] = (winner[0], winner[1], winner[4])
        for c in cands:
            if c is not winner:
                supplements.append((c[0], "id", c[1], c[2], c[4]))

    # All numeric_id constants are carried explicitly in the supplement CSV. The
    # paired id constant (same name) is emitted from its node with the same
    # literal value, so the two forms stay consistent without cross-referencing.
    for domain, name, value in numeric_consts:
        supplements.append((domain, "numeric_id", name, value, "SCADA"))

    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "extra_node_ids.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["domain", "kind", "name", "value", "ns_const"])
        for row in sorted(supplements):
            w.writerow(row)
    print(f"nodes claimed: {len(claimed)}; supplements: {len(supplements)}; "
          f"hand-written specials: {len(specials)}")
    for s in specials:
        print(f"  special (keep hand-written): {s[0]}::id::{s[1]} = {s[2]} [{s[3]}]")

    # namespaces.csv from namespaces.h (const names) + namespaces.cpp (db names).
    write_namespaces_csv(args.headers, args.out, nsmap)

    if args.apply_xml:
        inject_xml(args.nodesets, claimed, DOMAIN_BY_XML)
        print("XML annotated in place.")


def write_namespaces_csv(headers, out, nsmap):
    # index -> const name (nsmap reversed), preserving gaps.
    idx_to_const = {v: k for k, v in nsmap.items()}
    db_names = []
    text = open(os.path.join(headers, "namespaces.cpp")).read()
    m = re.search(r"kNamespaceNames\[\]\s*=\s*\{(.*?)\};", text, re.S)
    for sm in re.finditer(r'"([^"]*)"', m.group(1)):
        db_names.append(sm.group(1))
    end = max(nsmap.values()) if nsmap else len(db_names)
    with open(os.path.join(out, "namespaces.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["index", "const_name", "db_name"])
        for i in range(len(db_names)):
            w.writerow([i, idx_to_const.get(i, ""), db_names[i]])
    print(f"namespaces.csv: {len(db_names)} rows")


def inject_xml(nodesets_dir, claimed, domain_by_xml):
    for path in sorted(glob.glob(os.path.join(nodesets_dir, "*.xml"))):
        fname = os.path.basename(path)
        file_domain = domain_by_xml.get(fname, "scada")
        text = open(path).read()

        def repl(m):
            body = m.group(1)
            self_closing = bool(m.group(2))
            attrs = dict(re.findall(r'(\w+)="([^"]*)"', body))
            idm = re.match(r"([A-Z0-9]+)\.(\d+)", attrs.get("id", ""))
            term = " />" if self_closing else ">"
            if not idm:
                return m.group(0)
            key = (NS_PREFIX.get(idm.group(1), -1), int(idm.group(2)))
            if key not in claimed or "symbolicName" in attrs:
                return m.group(0)
            domain, name, _ns = claimed[key]
            extra = f' symbolicName="{name}"'
            if domain != file_domain:
                extra += f' codeNs="{domain}"'
            # Insert right after browseName="..." for readability, else after id.
            anchor = re.search(r'browseName="[^"]*"', body)
            if not anchor:
                anchor = re.search(r'id="[^"]*"', body)
            pos = anchor.end()
            new_body = body[:pos] + extra + body[pos:]
            return "<Node " + new_body + term

        # Match opening and self-closing <Node ...> tags, capturing the trailing
        # slash so it is reproduced exactly (not duplicated).
        new_text = re.sub(r"<Node\s+([^>]*?)\s*(/?)>", repl, text)
        open(path, "w").write(new_text)


if __name__ == "__main__":
    main()
