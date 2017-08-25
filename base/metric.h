#pragma once

class Metric {
 public:
  Metric(Logger& logger, const std::string& name)
      : logger_(logger),
        name_(name),
        count_(0) { }

  void Update(unsigned value) {
    if (count_ == 0) {
      low_ = high_ = sum_ = value;
    } else {
      low_ = std::min(low_, value);
      high_ = std::max(high_, value);
      sum_ += value;
    }
    last_ = value;
    ++count_;

    if (base::TimeTicks::Now() - last_report_ticks_ >= base::TimeDelta::FromMinutes(1)) {
      last_report_ticks_ = base::TimeTicks::Now();
      Report();
      Reset();
    }
  }

  void Reset() {
    count_ = 0;
  }

  void Report() {
    if (count_ == 0)
      return;

    unsigned avg = sum_ / count_;
    logger_.WriteF(LogSeverity::Normal, "%s = { last: %u, avg: %u, min: %u, max: %u, count: %u }",
        name_.c_str(), last_, avg, low_, high_, count_);
  }

 private:
  Logger& logger_;
  std::string name_;

  unsigned low_;
  unsigned high_;
  unsigned sum_;
  unsigned last_;
  size_t count_;

  base::TimeTicks last_report_ticks_;
};
