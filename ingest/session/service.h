#pragma once

#include "ingest/common/base_service.h"
#include "ingest/common/events.h"
#include "ingest/common/service_directory.h"
#include "ingest/proto/session.pb.h"

namespace ingest {

class SessionService : public BaseService, public EventEmitter<Session> {
public:
  void init(const ServiceDirectory&) override {}
  void run(const Session& session);
};

}
