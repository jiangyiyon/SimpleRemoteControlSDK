#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "screensdk/capture/dxgi_capture.h"
#include "screensdk/capture/display_detector.h"
#include "screensdk/encoding/encoder_factory.h"
#include "screensdk/encoding/encoder_config.h"
#include "screensdk/transport/data_channel.h"
#include "screensdk/utils/metrics_collector.h"

namespace screensdk {

/**
 * @brief Video frame with timestamp for latency measurement
 */
struct TimestampedFrame {
  std::vector<uint8_t> data;
  uint64_t capture_time_ms;
  uint32_t frame_number;
};

/**
 * @brief Video frame protocol for transmission
 */
#pragma pack(push, 1)
struct VideoFrameHeader {
  uint32_t magic{0x56465231};  // "VFR1"
  uint32_t frame_number{0};
  uint64_t timestamp_ms{0};
  uint32_t width{0};
  uint32_t height{0};
  uint32_t data_size{0};
};
#pragma pack(pop)

/**
 * @brief Simulated controller (client side)
 */
class MockController {
public:
  MockController();
  ~MockController();

  bool initialize();
  void cleanup();

  DataChannel* getDataChannel() { return data_channel_.get(); }

  void onDataReceived(const std::vector<uint8_t>& data);
  void waitForConnection(int timeout_ms = 5000);
  void waitForFrame(int timeout_ms = 1000);

  int getReceivedFrameCount() const;
  uint64_t getLastFrameLatency();
  TimestampedFrame getLastFrame();

private:
  std::unique_ptr<DataChannel> data_channel_;
  mutable std::mutex frame_mutex_;
  std::condition_variable frame_cv_;
  std::condition_variable connected_cv_;
  std::vector<TimestampedFrame> received_frames_;
  bool connected_{false};
  int frame_count_{0};
};

/**
 * @brief Simulated remote host (server side)
 */
class MockRemoteHost {
public:
  MockRemoteHost();
  ~MockRemoteHost();

  bool initialize(int display_index = 0);
  void startStreaming();
  void stopStreaming();
  void cleanup();

  DataChannel* getDataChannel() { return data_channel_.get(); }

  void waitForConnection(int timeout_ms = 5000);

  int getSentFrameCount() const;

private:
  void sendVideoFrame(const VideoFrame& frame);
  void onFrameCaptured(const VideoFrame& frame);

  std::unique_ptr<DataChannel> data_channel_;
  std::unique_ptr<DxgiCapture> capture_;
  std::unique_ptr<IEncoderFactory> encoder_factory_;
  IVideoEncoder* encoder_{nullptr};
  std::unique_ptr<DisplayDetector> display_detector_;

  mutable std::mutex send_mutex_;
  std::condition_variable connected_cv_;
  bool connected_{false};
  bool streaming_{false};

  uint32_t frame_number_{0};
  int sent_frame_count_{0};

  std::vector<uint8_t> encode_buffer_;
};

/**
 * @brief Helper to establish P2P connection
 */
class ConnectionHelper {
public:
  static bool establishConnection(MockRemoteHost& remote,
                                  MockController& controller,
                                  int timeout_ms = 10000);

private:
  static bool exchangeSdpAndIce(DataChannel& remote_dc,
                                DataChannel& controller_dc,
                                int timeout_ms);
};

} // namespace screensdk
