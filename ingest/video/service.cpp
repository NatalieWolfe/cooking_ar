#include "ingest/video/service.h"

#include "ingest/session/service.h"

namespace ingest {

void VideoService::init(const ServiceDirectory& directory) {
  directory.get<SessionService>().on_emit(*this);
}

void VideoService::handle(const Session& session) {}

}
