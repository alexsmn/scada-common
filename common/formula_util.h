#pragma once

#include "model/node_id_util.h"
#include "scada/node_id.h"

#include <string>
#include <string_view>

std::string GetFormulaSingleName(std::string_view formula);
scada::NodeId GetFormulaSingleNodeId(std::string_view formula);
std::string MakeNodeIdFormula(const scada::NodeId& id);

// A formula naming `id` that may be stored: NodeIdToStoredString's text,
// braced when it is the OPC UA Part 6 form (`{nsu=<uri>;s=Plant.Temperature}`)
// and otherwise exactly MakeNodeIdFormula(id). Never braces a string
// identifier holding `}`, which would end the operand early: that gives an
// empty formula, as an unsupported identifier type does.
std::string MakeStoredNodeIdFormula(
    const scada::NodeId& id,
    const scada::NamespaceUriForIndex& uri_for_index);

// Rewrites the node references in a formula, or in a bare node reference, for
// storage: each one in a namespace the model has no short name for -- written
// `NS<n>.…` this session, `n` being an index only this session gave -- becomes
// the Part 6 `nsu=` form. A reference is the whole text when it is a single
// name (after an optional leading `=`, as a sheet cell has), and each braced
// operand otherwise; an unbraced `NS<n>.…` inside a larger expression is left
// as written. Everything else is kept byte for byte.
std::string FormulaReferencesToStored(
    std::string_view formula,
    const scada::NamespaceUriForIndex& uri_for_index);

// The inverse of FormulaReferencesToStored, for a formula read back from
// storage: each `nsu=` reference to a namespace the model has no short name
// for becomes this session's `NS<n>.…` form, which every reader of a window
// item takes. A reference written in any other form, or naming nothing this
// connection can resolve, is kept as written.
std::string FormulaReferencesFromStored(
    std::string_view formula,
    const scada::NamespaceIndexForUri& index_for_uri);

std::string_view GetParentGroupChannelPath(std::string_view path);
