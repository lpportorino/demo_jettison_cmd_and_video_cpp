// status_display.cpp - Live status display implementation

#include "status_display.h"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace jettison
{

StatusDisplay::StatusDisplay (const std::string &camera_name,
                                const std::string &hostname,
                                const std::string &endpoint,
                                const std::string &rtsp_url)
    : camera_name_ (camera_name), hostname_ (hostname), endpoint_ (endpoint),
      rtsp_url_ (rtsp_url)
{
}

void
StatusDisplay::update_frame (uint32_t sequence, uint64_t duration_ns,
                              uint64_t system_time_ns)
{
  sequence_ = sequence;
  duration_ns_ = duration_ns;
  system_time_ns_ = system_time_ns;
}

void
StatusDisplay::print_full_status () const
{
  double system_time_s = system_time_ns_ / 1'000'000'000.0;
  double duration_ms = duration_ns_ / 1'000'000.0;
  double fps = calculate_fps (duration_ns_);
  std::string uptime = format_uptime (system_time_ns_);

  std::cout << "\n";
  std::cout
      << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << camera_name_ << " Camera Control - " << hostname_ << "\n";
  std::cout
      << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "Endpoint:    wss://" << hostname_ << ":443" << endpoint_
            << "\n";
  std::cout << "RTSP Stream: " << rtsp_url_ << "\n";
  std::cout << std::fixed << std::setprecision (1);
  std::cout << "Frame Rate:  " << fps << " FPS\n";
  std::cout
      << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "\n";

  print_status_line ();
  std::cout << "\n";
}

void
StatusDisplay::print_status_line () const
{
  double system_time_s = system_time_ns_ / 1'000'000'000.0;
  double duration_ms = duration_ns_ / 1'000'000.0;
  std::string uptime = format_uptime (system_time_ns_);

  std::cout << std::fixed << std::setprecision (3);
  std::cout << "Frame: " << sequence_ << "  |  System Time: " << system_time_s
            << "s  |  Duration: " << duration_ms << "ms\n";
  std::cout << "Uptime: " << uptime << "\n";
}

std::string
StatusDisplay::format_uptime (uint64_t system_time_ns) const
{
  uint64_t uptime_s = system_time_ns / 1'000'000'000;
  uint64_t hours = uptime_s / 3600;
  uint64_t minutes = (uptime_s % 3600) / 60;
  uint64_t seconds = uptime_s % 60;

  std::ostringstream oss;
  oss << hours << "h " << minutes << "m " << seconds << "s";
  return oss.str ();
}

double
StatusDisplay::calculate_fps (uint64_t duration_ns) const
{
  if (duration_ns == 0)
    {
      return 0.0;
    }
  return 1.0 / (duration_ns / 1'000'000'000.0);
}

} // namespace jettison
