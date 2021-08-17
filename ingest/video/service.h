#pragma once

#include "ingest/common/base_service.h"
#include "ingest/common/events.h"
#include "ingest/proto/session.pb.h"
#include "ingest/proto/video.pb.h"

namespace ingest {

class VideoService :
  public BaseService,
  public EventEmitter<VideoFrame>,
  public EventHandler<Session> {
public:
  void init(const ServiceDirectory& directory) override;
  void handle(const Session& session) override;
};

}
