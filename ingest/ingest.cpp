#include "ingest/common/service_directory.h"
#include "ingest/session/service.h"
#include "ingest/video/service.h"

int main() {
  ingest::ServiceDirectory directory;
  ingest::SessionService sessions;
  ingest::VideoService videos;

  directory.insert(sessions);
  directory.insert(videos);

  videos.init(directory);

  return 0;
}
