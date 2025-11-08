// status_display.h - Live status display for frame counter and timing info
// Shows current frame, timing, uptime during command loop

#ifndef JETTISON_STATUS_DISPLAY_H
#define JETTISON_STATUS_DISPLAY_H

#include <cstdint>
#include <string>

namespace jettison
{

// Live status display
class StatusDisplay
{
public:
  // Constructor
  // camera_name: "HEAT" or "DAY"
  // hostname: Device hostname (e.g., "sych.local")
  // endpoint: WebSocket video endpoint (e.g., "/ws/ws_video_heat")
  // rtsp_url: RTSP stream URL (e.g., "rtsp://localhost:8554/heat")
  StatusDisplay (const std::string &camera_name, const std::string &hostname,
                 const std::string &endpoint, const std::string &rtsp_url);

  ~StatusDisplay () = default;

  // Non-copyable, non-movable
  StatusDisplay (const StatusDisplay &) = delete;
  StatusDisplay &operator= (const StatusDisplay &) = delete;
  StatusDisplay (StatusDisplay &&) = delete;
  StatusDisplay &operator= (StatusDisplay &&) = delete;

  // Update frame data
  void update_frame (uint32_t sequence, uint64_t duration_ns,
                     uint64_t system_time_ns);

  // Print full status display (header + current frame info)
  void print_full_status () const;

  // Print one-line status update (just frame counter line)
  void print_status_line () const;

private:
  std::string camera_name_;
  std::string hostname_;
  std::string endpoint_;
  std::string rtsp_url_;

  // Current frame data
  uint32_t sequence_{ 0 };
  uint64_t duration_ns_{ 0 };
  uint64_t system_time_ns_{ 0 };

  // Helper to format uptime string
  std::string format_uptime (uint64_t system_time_ns) const;

  // Helper to calculate FPS
  double calculate_fps (uint64_t duration_ns) const;
};

} // namespace jettison

#endif // JETTISON_STATUS_DISPLAY_H
