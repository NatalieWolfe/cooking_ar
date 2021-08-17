#pragma once

#include "ingest/common/service_directory.h"

namespace ingest {

class BaseService {
public:
  BaseService() = default;
  virtual ~BaseService() = default;
  BaseService(BaseService&&) = default;
  BaseService& operator=(BaseService&&) = default;

  BaseService(const BaseService&) = delete;
  BaseService& operator=(const BaseService&) = delete;

  virtual void init(const ServiceDirectory& directory) = 0;
};

}
