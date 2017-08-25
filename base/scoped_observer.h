#pragma once

template<class Observable, class Observer>
class ScopedObserver {
 public:
  ScopedObserver(Observable& observable, Observer& observer)
      : observable_(observable),
        observer_(observer) {
    observable_.AddObserver(observer_);
  }

  ~ScopedObserver() {
    observable_.RemoveObserver(observer_);
  }

 private:
  Observable& observable_;
  Observer& observer_;
};
