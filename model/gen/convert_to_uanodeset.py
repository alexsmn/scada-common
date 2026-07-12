#!/usr/bin/env python3
"""Phase-1 one-shot converter: scada-node-state-v1 -> OPC UA UANodeSet2.

Reads the custom nodeset XML (common/model/nodesets/*.xml) and emits a standard
UANodeSet2 file for the SCADA namespace (file-local ns=1), referencing OPC UA
namespace-0 nodes as i=. Numeric ids are preserved verbatim -- this is a syntax
transform, not a renumbering. See common/model/docs/uanodeset-migration.md.

This is a development tool, not part of the build. It produces review artifacts
for the UANodeSet2 migration; the runtime loader still reads the custom format
until Phase 2 lands.
"""
import argparse
import glob
import os
import re
from xml.sax.saxutils import escape, quoteattr

UANODESET_NS = "http://opcfoundation.org/UA/2011/03/UANodeSet.xsd"
UAX_NS = "http://opcfoundation.org/UA/2008/02/Types.xsd"
OPCUA_URI = "http://opcfoundation.org/UA/"

# custom class -> UANodeSet element name
CLASS_ELEM = {
    "ObjectType": "UAObjectType", "VariableType": "UAVariableType",
    "DataType": "UADataType", "ReferenceType": "UAReferenceType",
    "Object": "UAObject", "Variable": "UAVariable",
    "Method": "UAMethod", "View": "UAView",
}
INSTANCE_ELEMS = {"UAObject", "UAVariable", "UAMethod", "UAView"}
VALUE_ELEMS = {"UAVariable", "UAVariableType"}

# Well-known namespace-0 numeric ids -> standard OPC UA BrowseName (used as
# aliases for ReferenceType / DataType attributes). Only the ids actually
# referenced by the nodesets need to be here.
NS0_ALIAS = {
    1: "Boolean", 2: "SByte", 6: "Int32", 8: "Int64", 9: "UInt64",
    11: "Double", 12: "String", 13: "DateTime", 15: "ByteString", 17: "NodeId",
    21: "LocalizedText", 24: "BaseDataType",
    32: "NonHierarchicalReferences", 33: "HierarchicalReferences",
    35: "Organizes", 37: "HasModellingRule", 40: "HasTypeDefinition",
    44: "Aggregates", 45: "HasSubtype", 46: "HasProperty", 47: "HasComponent",
}
# custom AttributeValue/@type token -> uax: element name
UAX_TYPE = {
    "Bool": "Boolean", "SByte": "SByte", "Int16": "Int16", "Int32": "Int32",
    "Int64": "Int64", "Byte": "Byte", "UInt16": "UInt16", "UInt32": "UInt32",
    "UInt64": "UInt64", "Float": "Float", "Double": "Double",
    "String": "String", "DateTime": "DateTime",
}


def parse_id(raw):
    """'NS0.40' -> (0, 40); 'SCADA.115' -> (7, 115). Returns (ns, num)."""
    m = re.match(r"(NS0|SCADA)\.(\d+)$", raw or "")
    if not m:
        return None
    return (0 if m.group(1) == "NS0" else 7, int(m.group(2)))


def nodeid_str(raw, scada_ns):
    """Emit a UANodeSet NodeId string; SCADA maps to the file-local index."""
    p = parse_id(raw)
    return f"i={p[1]}" if p[0] == 0 else f"ns={scada_ns};i={p[1]}"


def reftype_str(raw, scada_ns, used_aliases):
    """ReferenceType/DataType as a standard alias when known, else a NodeId."""
    p = parse_id(raw)
    if p and p[0] == 0 and p[1] in NS0_ALIAS:
        used_aliases.add(p[1])
        return NS0_ALIAS[p[1]]
    return nodeid_str(raw, scada_ns)


def read_nodes(paths):
    """Return list of dicts, one per <Node>, across all input files."""
    nodes = []
    for path in paths:
        text = open(path, encoding="utf-8").read()
        for m in re.finditer(r"<Node\b(.*?)(/>|>(.*?)</Node>)", text, re.S):
            attrs = dict(re.findall(r'(\w+)="([^"]*)"', m.group(1)))
            body = m.group(3) or ""
            refs = []
            for rm in re.finditer(r"<Reference\b([^>]*?)/?>", body):
                ra = dict(re.findall(r'(\w+)="([^"]*)"', rm.group(1)))
                refs.append(ra)
            av = re.search(
                r'<AttributeValue\b([^>]*?)(?:/>|>(.*?)</AttributeValue>)', body, re.S)
            value = None
            if av:
                a = dict(re.findall(r'(\w+)="([^"]*)"', av.group(1)))
                value = (a.get("type"), (av.group(2) or "").strip())
            nodes.append({"attrs": attrs, "refs": refs, "value": value})
    return nodes


def build_references(n, scada_ns, used_aliases):
    """Deduped reference set for a node, matching the loader's union rules."""
    a = n["attrs"]
    out = []
    seen = set()

    def add(reftype_raw, target_raw, forward):
        rt = reftype_str(reftype_raw, scada_ns, used_aliases)
        tg = nodeid_str(target_raw, scada_ns)
        key = (rt, tg, forward)
        if key in seen:
            return
        seen.add(key)
        out.append((rt, tg, forward))

    if a.get("supertype"):
        add("NS0.45", a["supertype"], False)          # supertype -> HasSubtype
    if a.get("parent") and a.get("parentReference"):
        add(a["parentReference"], a["parent"], False)  # parent (inverse)
    if a.get("typeDefinition"):
        add("NS0.40", a["typeDefinition"], True)       # HasTypeDefinition
    for r in n["refs"]:
        if r.get("type") and r.get("target"):
            add(r["type"], r["target"], r.get("forward", "true") != "false")
    return out


def emit(nodes, scada_uri, pub_date, ns0_pub_date):
    scada_ns = 1  # file-local index for the SCADA namespace
    used_aliases = set()
    body = []
    for n in nodes:
        a = n["attrs"]
        p = parse_id(a.get("id"))
        if not p or p[0] != 7:      # emit only SCADA-namespace nodes
            continue
        elem = CLASS_ELEM.get(a.get("class"))
        if not elem:
            continue
        el_attrs = [
            f'NodeId="{nodeid_str(a["id"], scada_ns)}"',
            f'BrowseName="{scada_ns}:{escape(a.get("browseName", ""))}"',
        ]
        if a.get("symbolicName"):
            el_attrs.append(f'SymbolicName="{escape(a["symbolicName"])}"')
        if elem in INSTANCE_ELEMS and a.get("parent"):
            el_attrs.append(f'ParentNodeId="{nodeid_str(a["parent"], scada_ns)}"')
        if elem in VALUE_ELEMS and a.get("dataType"):
            el_attrs.append(
                f'DataType="{reftype_str(a["dataType"], scada_ns, used_aliases)}"')

        lines = [f'  <{elem} ' + " ".join(el_attrs) + ">"]
        lines.append(f'    <DisplayName>{escape(a.get("displayName", ""))}</DisplayName>')
        refs = build_references(n, scada_ns, used_aliases)
        if refs:
            lines.append("    <References>")
            for rt, tg, fwd in refs:
                fwd_attr = "" if fwd else ' IsForward="false"'
                lines.append(
                    f'      <Reference ReferenceType={quoteattr(rt)}{fwd_attr}>{tg}</Reference>')
            lines.append("    </References>")
        if elem in VALUE_ELEMS and n["value"] and n["value"][0] in UAX_TYPE:
            uax = UAX_TYPE[n["value"][0]]
            lines.append(
                f'    <Value><uax:{uax}>{escape(n["value"][1])}</uax:{uax}></Value>')
        lines.append(f"  </{elem}>")
        body.append("\n".join(lines))

    aliases = "\n".join(
        f'    <Alias Alias="{NS0_ALIAS[i]}">i={i}</Alias>'
        for i in sorted(used_aliases))
    header = f'''<?xml version="1.0" encoding="UTF-8"?>
<UANodeSet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
           xmlns:uax="{UAX_NS}"
           xmlns="{UANODESET_NS}">
  <NamespaceUris>
    <Uri>{escape(scada_uri)}</Uri>
  </NamespaceUris>
  <Models>
    <Model ModelUri="{escape(scada_uri)}" Version="1.0.0" PublicationDate="{pub_date}">
      <RequiredModel ModelUri="{OPCUA_URI}" Version="1.05.03" PublicationDate="{ns0_pub_date}"/>
    </Model>
  </Models>
  <Aliases>
{aliases}
  </Aliases>'''
    return header + "\n" + "\n".join(body) + "\n</UANodeSet>\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--nodesets", default="common/model/nodesets")
    ap.add_argument("--out", required=True)
    ap.add_argument("--scada-uri", default="http://telecontrol.example/SCADA/")
    ap.add_argument("--pub-date", default="2026-01-01T00:00:00Z")
    ap.add_argument("--ns0-pub-date", default="2023-12-15T00:00:00Z")
    ap.add_argument("--files", nargs="*",
                    help="specific nodeset files (default: all *.xml)")
    args = ap.parse_args()

    paths = ([os.path.join(args.nodesets, f) for f in args.files]
             if args.files else sorted(glob.glob(os.path.join(args.nodesets, "*.xml"))))
    nodes = read_nodes(paths)
    out = emit(nodes, args.scada_uri, args.pub_date, args.ns0_pub_date)
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    open(args.out, "w", encoding="utf-8").write(out)
    scada_count = sum(1 for n in nodes
                      if (parse_id(n["attrs"].get("id")) or (0,))[0] == 7)
    print(f"read {len(nodes)} nodes, emitted {scada_count} SCADA nodes -> {args.out}")


if __name__ == "__main__":
    main()
