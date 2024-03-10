#pragma once

#include "timed_data/timed_data_context.h"
#include "timed_data/timed_data_service.h"

#include <memory>

std::unique_ptr<TimedDataService> CreateTimedDataService(
    TimedDataContext&& context);