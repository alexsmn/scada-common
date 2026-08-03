#include "model/static_nodesets.h"

namespace scada {

std::filesystem::path GetScadaStaticNodesetSourceDir() {
  return std::filesystem::path{__FILE__}.parent_path() / "nodesets";
}

std::vector<std::filesystem::path> GetScadaStaticNodesetSourcePaths() {
  const auto dir = GetScadaStaticNodesetSourceDir();
  std::vector<std::filesystem::path> paths;
  paths.reserve(kScadaStaticNodesetFiles.size());
  for (const auto& name : kScadaStaticNodesetFiles) {
    paths.push_back(dir / name);
  }
  return paths;
}

}  // namespace scada
