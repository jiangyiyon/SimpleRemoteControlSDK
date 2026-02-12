#include "screensdk/transport/video_source.h"

namespace screensdk {

std::unique_ptr<IVideoSource> createVideoSource() {
  // TODO: Implement with DXGI capture integration
  // This will be connected to dxgi_capture when available
  return nullptr;
}

} // namespace screensdk
