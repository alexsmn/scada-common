#pragma once

#include "core/view_service.h"

#include <gmock/gmock.h>

namespace scada {

class ViewServiceMock : public ViewService {
 public:
  MOCK_METHOD2(Browse, void(const std::vector<BrowseDescription>& descriptions, const BrowseCallback& callback));

  MOCK_METHOD1(Subscribe, void(ViewEvents& events));
  MOCK_METHOD1(Unsubscribe, void(ViewEvents& events));
};

} // namespace scada
