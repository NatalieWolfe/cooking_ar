#pragma once

#include <typeindex>
#include <unordered_map>

namespace ingest {

class BaseService;

class ServiceDirectory {
public:
  template <typename Service>
  void insert(Service& service) {
    _services.insert({std::type_index{typeid(Service)}, &service});
  }

  template <typename Service>
  Service& get() const {
    return static_cast<Service&>(
      *_services.at(std::type_index{typeid(Service)})
    );
  }

private:
  std::unordered_map<std::type_index, BaseService*> _services;
};

}
