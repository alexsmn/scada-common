#pragma once

#include "core/basic_types.h"
#include "core/string.h"

namespace scada {

class QualifiedName {
 public:
  QualifiedName() {}

  QualifiedName(String name, NamespaceIndex namespace_index = 0)
      : namespace_index_{namespace_index},
        name_{std::move(name)} {
  }

  NamespaceIndex namespace_index() const { return namespace_index_; }
  const String& name() const { return name_; }
  bool empty() const { return namespace_index_ == 0 && name_.empty(); }

  bool operator==(const QualifiedName& other) const {
    return namespace_index_ == other.namespace_index_ && name_ == other.name_;
  }

  bool operator!=(const QualifiedName& other) const { return !operator==(other); }

 private:
  NamespaceIndex namespace_index_ = 0;
  String name_;
};

} // namespace scada