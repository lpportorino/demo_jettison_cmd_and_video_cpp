// rtsp_server.h - GStreamer RTSP server for local video streaming
// Accepts H.264 AVCC data and streams via RTSP/RTP

#ifndef JETTISON_RTSP_SERVER_H
#define JETTISON_RTSP_SERVER_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace jettison
{

// GStreamer RTSP server
class RtspServer
{
public:
  // Constructor
  // stream_name: Stream name (e.g., "heat" or "day")
  // port: RTSP server port (e.g., 8554 for heat, 8555 for day)
  RtspServer (const std::string &stream_name, int port);

  ~RtspServer ();

  // Non-copyable, non-movable
  RtspServer (const RtspServer &) = delete;
  RtspServer &operator= (const RtspServer &) = delete;
  RtspServer (RtspServer &&) = delete;
  RtspServer &operator= (RtspServer &&) = delete;

  // Start RTSP server
  // Returns true if server starts successfully, false otherwise
  bool start ();

  // Stop RTSP server
  void stop ();

  // Push H.264 AVCC frame to RTSP server
  // data: H.264 AVCC data (length-prefixed NAL units)
  // size: Size of data in bytes
  // pts_ns: Presentation timestamp in nanoseconds
  // Returns true if frame pushed successfully, false otherwise
  bool push_frame (const uint8_t *data, size_t size, uint64_t pts_ns);

  // Get RTSP URL
  std::string get_url () const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace jettison

#endif // JETTISON_RTSP_SERVER_H
