#include "lf/queue.h"

#include <any>
#include <optional>
#include <stdexcept>

namespace lf::impl {
namespace {

constexpr std::size_t incr(std::size_t num, std::size_t limit) {
  return (num + 1) % limit;
}

constexpr std::size_t decr(std::size_t num, std::size_t limit) {
  return (num + limit - 1) % limit;
}

}

QueueBase::QueueBase(std::size_t capacity):
  _capacity{capacity + 1},
  _push_idx{0},
  _pop_idx{_capacity - 1},
  _push_limit{_capacity - 1},
  _pop_limit{_capacity - 1}
{
  _queue.resize(_capacity);
}

bool QueueBase::empty() const {
  return _push_idx == incr(_pop_idx, _capacity);
}

std::optional<std::any> QueueBase::pop() {
  std::size_t pop_idx = 0;
  std::size_t next_pop_idx = 0;

  // Loop until we successfully claim a pop index or find we are empty.
  do {
    if (empty()) return std::nullopt;
    // We are not empty, but may need to wait for the value to be moved in.
    while (_pop_idx == _pop_limit) continue;

    // Attempt to claim the index.
    pop_idx = _pop_idx;
    next_pop_idx = incr(pop_idx, _capacity);
  } while (
    !_pop_idx.compare_exchange_weak(pop_idx, next_pop_idx)
  );

  // Move the value out of the queue and then increment our push limit.
  std::optional<std::any> ret = std::move(_queue.at(next_pop_idx));
  std::size_t prev_push_limit = decr(next_pop_idx, _capacity);
  while (!_push_limit.compare_exchange_weak(prev_push_limit, next_pop_idx)) {
    continue;
  }
  return ret;
}

void QueueBase::push(std::any value) {
  std::any* insert_pos = nullptr;
  std::size_t push_idx = 0;
  std::size_t next_push_idx = 0;

  // Loop until we successfully claim a push index.
  do {
    // If we're full, abort!
    push_idx = _push_idx;
    next_push_idx = incr(push_idx, _capacity);
    if (push_idx == _pop_idx) {
      throw std::length_error("Queue capacity reached.");
    }
    // We're not full, but we might need to wait for the last popped item to be
    // moved out of the queue.
    while (_push_idx == _push_limit) continue;

    // Attempt to claim the index.
    insert_pos = &_queue.at(push_idx);
  } while (!_push_idx.compare_exchange_weak(push_idx, next_push_idx));

  // Insert the value and then increment the pop limit.
  *insert_pos = std::move(value);
  std::size_t prev_pop_limit = decr(push_idx, _capacity);
  while (!_pop_limit.compare_exchange_weak(prev_pop_limit, push_idx)) {
    continue;
  }
}

std::size_t QueueBase::size() const {
  std::size_t pop_idx = incr(_pop_idx, _capacity);
  std::size_t push_idx = _push_idx;
  if (push_idx >= pop_idx) return push_idx - pop_idx;
  return _capacity - pop_idx + push_idx;
}

}
