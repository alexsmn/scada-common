#include "common/common_paths.h"

#include "base/path_service.h"

#include <filesystem>

namespace scada {

bool PathProvider(int key, std::filesystem::path* result) {
/*  // Assume that we will not need to create the directory if it does not exist.
  // This flag can be set to true for the cases where we want to create it.
  bool create_dir = false;

  std::filesystem::path cur;
  switch (key) {
    case DIR_LOG:
      if (!PathService::Get(base::DIR_LOCAL_APP_DATA, &cur))
        return false;
      cur = cur / "logs";
      create_dir = true;
      break;

    default:
      return false;
  }

  if (create_dir && !std::filesystem::exists(cur) && !std::filesystem::create_directories(cur))
    return false;

  *result = cur;
  return true;*/

  return false;
}

void RegisterPathProvider() {
//  PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}

} // namespace scada
