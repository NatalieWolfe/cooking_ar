#include "ingest/session/service.h"

#include "ingest/proto/session.pb.h"

namespace ingest {

void SessionService::run(const Session& session) {
  emit(session);
}

}
