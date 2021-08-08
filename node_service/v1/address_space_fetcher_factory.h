#pragma once

#include <functional>

namespace scada {
struct ModelChangeEvent;
struct SemanticChangeEvent;
}  // namespace scada

namespace v1 {

class AddressSpaceFetcher;
struct NodeFetchStatusChangedItem;

using NodeFetchStatusChangedHandler =
    std::function<void(base::span<const NodeFetchStatusChangedItem> items)>;

struct AddressSpaceFetcherFactoryContext {
  const NodeFetchStatusChangedHandler node_fetch_status_changed_handler_;

  const std::function<void(const scada::ModelChangeEvent& event)>
      model_changed_handler_;

  const std::function<void(const scada::SemanticChangeEvent& event)>
      semantic_changed_handler_;
};

using AddressSpaceFetcherFactory =
    std::function<std::shared_ptr<AddressSpaceFetcher>(
        AddressSpaceFetcherFactoryContext&& context)>;

}  // namespace v1
