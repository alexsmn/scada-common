#pragma once

#include "common/aliases.h"

#include <gmock/gmock.h>

using MockAliasResolver =
    testing::MockFunction<void(std::string_view alias,
                               const AliasResolveCallback& callback)>;
