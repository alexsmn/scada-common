#include "common/common_paths.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"

namespace scada {

bool PathProvider(int key, base::FilePath* result) {
/*  // Assume that we will not need to create the directory if it does not exist.
  // This flag can be set to true for the cases where we want to create it.
  bool create_dir = false;

  base::FilePath cur;
  switch (key) {
    case DIR_LOG:
      if (!PathService::Get(base::DIR_LOCAL_APP_DATA, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("logs"));
      create_dir = true;
      break;

    default:
      return false;
  }

  if (create_dir && !base::PathExists(cur) && !base::CreateDirectory(cur))
    return false;

  *result = cur;
  return true;*/

  return false;
}

void RegisterPathProvider() {
//  PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}

} // namespace scada
