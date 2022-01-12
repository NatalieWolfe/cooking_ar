#pragma once

#include <any>
#include <atomic>
#include <optional>
#include <vector>

namespace lf {
namespace impl {

class QueueBase {
public:
  explicit QueueBase(std::size_t capacity);

  std::optional<std::any> pop();
  void push(std::any value);

  std::size_t size() const;

  [[nodiscard]] bool empty() const;
  std::size_t max_size() const { return _capacity - 1; }
  std::size_t capacity() const { return max_size(); }

private:
  std::vector<std::any> _queue;
  const std::size_t _capacity;
  std::atomic_size_t _push_idx;
  std::atomic_size_t _pop_idx;
  std::atomic_size_t _push_limit;
  std::atomic_size_t _pop_limit;
};

}

template <typename T>
class Queue : private impl::QueueBase {
public:
  explicit Queue(std::size_t capacity): impl::QueueBase{capacity} {}

  std::optional<T> pop() {
    std::optional<std::any> value = impl::QueueBase::pop();
    if (!value) return std::nullopt;
    return std::any_cast<T>(*std::move(value));
  }

  void push(T value) {
    impl::QueueBase::push(std::make_any<T>(std::move(value)));
  }

  using impl::QueueBase::empty;
  using impl::QueueBase::size;
  using impl::QueueBase::max_size;
  using impl::QueueBase::capacity;
};

}
