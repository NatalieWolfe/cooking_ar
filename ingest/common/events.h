#pragma once

#include <unordered_set>

namespace ingest {

template <typename Event>
class EventHandler {
public:
  virtual ~EventHandler() = default;

  virtual void handle(const Event& e) = 0;
};

template <typename Event>
class EventEmitter {
public:
  virtual ~EventEmitter() = default;

  void on_emit(EventHandler<Event>& handler) {
    _handlers.insert(&handler);
  }

  void emit(const Event& event) {
    for (EventHandler<Event>* handler : _handlers) handler->handle(event);
  }
private:
  std::unordered_set<EventHandler<Event>*> _handlers;
};

}
