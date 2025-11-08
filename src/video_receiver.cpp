// video_receiver.cpp - Video receiver thread implementation

#include "video_receiver.h"

#include <cstring>
#include <iostream>

namespace jettison
{

VideoReceiver::VideoReceiver (const std::string &hostname, int port,
                               const std::string &path, SharedTiming &timing,
                               VideoWriter &video_writer,
                               std::atomic<bool> &running)
    : ws_client_ (hostname, port, path), timing_ (timing),
      video_writer_ (video_writer), running_ (running)
{
  // Set message callback
  ws_client_.set_message_callback (
      [this] (const uint8_t *data, size_t len)
      { this->on_video_frame (data, len); });
}

void
VideoReceiver::run ()
{
  std::cout << "Connecting to video stream...\n";

  // Connect to WebSocket
  if (!ws_client_.connect ())
    {
      std::cerr << " Failed to connect to video stream\n";
      running_.store (false);
      return;
    }

  std::cout << " Connected to video stream\n";

  // Service WebSocket until disconnect
  while (running_.load () && ws_client_.is_connected ())
    {
      ws_client_.service (50); // 50ms timeout
    }

  // Disconnected
  if (!running_.load ())
    {
      std::cout << " Video receiver thread stopped\n";
    }
  else
    {
      std::cerr << " Video stream disconnected\n";
      running_.store (false);
    }
}

void
VideoReceiver::on_video_frame (const uint8_t *data, size_t len)
{
  // Video frame format (from web frontend videoSubDecoder.js):
  // Header (24 bytes):
  //   [0:8]    uint64    PTS timestamp (microseconds, little-endian)
  //   [8:16]   uint64    Frame duration (microseconds, little-endian)
  //   [16:24]  uint64    System monotonic time (microseconds, CLOCK_MONOTONIC, little-endian)
  //
  // Video Data (offset 24+):
  //   Format: Raw H.264 NAL units (Annex B format with start codes)

  if (len < 24)
    {
      std::cerr << "⚠ Video frame too short: " << len << " bytes\n";
      return;
    }

  // Parse header (24 bytes)
  uint64_t pts_us = read_uint64_le (data + 0);           // PTS timestamp
  uint64_t duration_us = read_uint64_le (data + 8);      // Frame duration
  uint64_t system_time_us = read_uint64_le (data + 16);  // System time

  // Convert to nanoseconds
  uint64_t pts_ns = pts_us * 1000;
  uint64_t duration_ns = duration_us * 1000;
  uint64_t system_time_ns = system_time_us * 1000;

  // Update shared timing (atomic, lock-free)
  // frame_time_ns = PTS, system_time_ns = CLOCK_MONOTONIC
  timing_.update (pts_ns, system_time_ns);

  // Store for statistics
  last_duration_ns_ = duration_ns;
  frame_count_++;

  // Write raw H.264 data to named pipe (offset 24+)
  size_t h264_size = len - 24;
  video_writer_.write_frame (data + 24, h264_size);

  // Print frame info periodically (every 30 frames = ~1 second at 30 FPS)
  if (frame_count_ % 30 == 0)
    {
      double system_time_s = system_time_ns / 1'000'000'000.0;
      double duration_ms = duration_ns / 1'000'000.0;
      double fps = 1'000'000'000.0 / duration_ns;

      std::cout << "\rFrame: " << frame_count_
                << "  |  Time: " << system_time_s << "s  |  Duration: "
                << duration_ms << "ms  |  FPS: " << fps << "    "
                << std::flush;
    }
}

uint64_t
VideoReceiver::read_uint64_le (const uint8_t *data)
{
  // Read little-endian uint64
  uint64_t value = 0;
  for (int i = 0; i < 8; i++)
    {
      value |= static_cast<uint64_t> (data[i]) << (i * 8);
    }
  return value;
}

} // namespace jettison
