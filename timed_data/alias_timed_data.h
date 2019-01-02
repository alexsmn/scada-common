#pragma once

#include "base/observer_list.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_spec.h"

#include <memory>

class AliasTimedData final : public rt::TimedData {
 public:
  explicit AliasTimedData(std::string formula);

  void SetForwarded(std::shared_ptr<rt::TimedData> timed_data);

  // rt::TimedData
  virtual bool IsError() const override;
  virtual base::Time GetReadyFrom() const override;
  virtual const std::vector<scada::DateTimeRange>& GetReadyRanges()
      const override;
  virtual scada::DataValue GetDataValue() const override;
  virtual scada::DataValue GetValueAt(const base::Time& time) const override;
  virtual base::Time GetChangeTime() const override;
  virtual const DataValues* GetValues() const override;
  virtual void AddObserver(rt::TimedDataDelegate& observer,
                           const scada::DateTimeRange& range) override;
  virtual void RemoveObserver(rt::TimedDataDelegate& observer) override;
  virtual std::string GetFormula(bool aliases) const override;
  virtual scada::LocalizedText GetTitle() const override;
  virtual NodeRef GetNode() const override;
  virtual bool IsAlerting() const override;
  virtual const events::EventSet* GetEvents() const override;
  virtual void Acknowledge() override;
  virtual void Write(double value,
                     const scada::NodeId& user_id,
                     const scada::WriteFlags& flags,
                     const StatusCallback& callback) const override;
  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback) const override;

 private:
  struct DeferredData {
    DeferredData(std::string formula) : formula{std::move(formula)} {}

    const std::string formula;
    std::map<rt::TimedDataDelegate*, scada::DateTimeRange /*range*/> observers;
  };

  bool is_forwarded() const { return data_.index() == 1; }

  DeferredData& deferred() {
    return *std::get<std::unique_ptr<DeferredData>>(data_);
  }

  const DeferredData& deferred() const {
    return const_cast<AliasTimedData*>(this)->deferred();
  }

  TimedData& forwarded() {
    return *std::get<std::shared_ptr<TimedData>>(data_);
  }

  const TimedData& forwarded() const {
    return const_cast<AliasTimedData*>(this)->forwarded();
  }

  std::variant<std::unique_ptr<DeferredData>, std::shared_ptr<rt::TimedData>>
      data_;
};
