// video_receiver.h - Video frame receiver and parser
// Receives video frames via WebSocket, parses headers, writes to named pipe

#ifndef JETTISON_VIDEO_RECEIVER_H
#define JETTISON_VIDEO_RECEIVER_H

#include "shared_timing.h"
#include "video_writer.h"
#include "websocket_client.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

namespace jettison
{

// Video receiver - runs in dedicated thread
// Parses 24-byte video frame headers and writes H.264 to named pipe
class VideoReceiver
{
public:
  // Constructor
  // hostname: Device hostname (e.g., "sych.local")
  // port: WebSocket port (443)
  // path: Video stream path (e.g., "/ws/ws_video_heat")
  // timing: Shared timing storage (written by this thread)
  // video_writer: Named pipe writer
  // running: Shared running flag (for exit)
  VideoReceiver (const std::string &hostname, int port,
                 const std::string &path, SharedTiming &timing,
                 VideoWriter &video_writer, std::atomic<bool> &running);

  ~VideoReceiver () = default;

  // Non-copyable, non-movable
  VideoReceiver (const VideoReceiver &) = delete;
  VideoReceiver &operator= (const VideoReceiver &) = delete;
  VideoReceiver (VideoReceiver &&) = delete;
  VideoReceiver &operator= (VideoReceiver &&) = delete;

  // Run video receiver thread (blocking)
  // Connects to WebSocket and processes video frames
  void run ();

  // Get frame statistics
  uint64_t get_frame_count () const { return frame_count_; }
  uint64_t get_last_duration_ns () const { return last_duration_ns_; }

private:
  // Video frame callback
  void on_video_frame (const uint8_t *data, size_t len);

  // Parse little-endian uint64
  static uint64_t read_uint64_le (const uint8_t *data);

  WebSocketClient ws_client_;
  SharedTiming &timing_;
  VideoWriter &video_writer_;
  std::atomic<bool> &running_;

  // Statistics
  uint64_t frame_count_ = 0;
  uint64_t last_duration_ns_ = 0;
};

} // namespace jettison

#endif // JETTISON_VIDEO_RECEIVER_H
