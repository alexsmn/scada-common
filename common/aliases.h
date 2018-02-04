#pragma once

#include "base/strings/string_piece.h"
#include "core/node_id.h"
#include "core/status.h"

#include <functional>

using AliasResolveCallback =
    std::function<void(scada::Status status, const scada::NodeId& node_id)>;
using AliasResolver = std::function<void(base::StringPiece alias,
                                         const AliasResolveCallback& callback)>;
