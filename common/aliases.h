#pragma once

#include "scada/node_id.h"
#include "scada/status.h"

#include <functional>
#include <string_view>

using AliasResolveCallback =
    std::function<void(scada::Status status, const scada::NodeId& node_id)>;
using AliasResolver = std::function<void(std::string_view alias,
                                         const AliasResolveCallback& callback)>;
