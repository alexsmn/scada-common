#pragma once

#include "core/basic_types.h"
#include "core/string.h"

namespace scada {

class QualifiedName {
 public:
  QualifiedName(String name, NamespaceIndex namespace_index)
      : name_{std::move(name)},
        namespace_index_{namespace_index} {
  }

  NamespaceIndex namespace_index() const { return namespace_index_; }
  const String& name() const { return name_; }

  bool operator==(const QualifiedName& other) const {
    return std::tie(namespace_index_, name_) == std::tie(other.namespace_index_, other.name_);
  }

  bool operator!=(const QualifiedName& other) const { return !operator==(other); }

 private:
  NamespaceIndex namespace_index_;
  String name_;
};

} // namespace scada