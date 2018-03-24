#pragma once

#include "base/strings/string16.h"
#include "core/basic_types.h"
#include "core/string.h"

namespace scada {

class QualifiedName {
 public:
  QualifiedName() {}

  QualifiedName(String name, NamespaceIndex namespace_index = 0)
      : namespace_index_{namespace_index}, name_{std::move(name)} {}

  QualifiedName(const char* name, NamespaceIndex namespace_index = 0)
      : namespace_index_{namespace_index}, name_{name} {}

  NamespaceIndex namespace_index() const { return namespace_index_; }
  const String& name() const { return name_; }
  bool empty() const { return namespace_index_ == 0 && name_.empty(); }

 private:
  String name_;
  NamespaceIndex namespace_index_ = 0;
};

inline bool operator==(const QualifiedName& a, const QualifiedName& b) {
  return a.namespace_index() == b.namespace_index() && a.name() == b.name();
}

inline bool operator!=(const QualifiedName& a, const QualifiedName& b) {
  return !operator==(a, b);
}

inline std::ostream& operator<<(std::ostream& stream,
                                const QualifiedName& value) {
  return stream << value.name();
}

}  // namespace scada

inline const std::string& ToString(const scada::QualifiedName& name) {
  return name.name();
}

base::string16 ToString16(const scada::QualifiedName& name);
