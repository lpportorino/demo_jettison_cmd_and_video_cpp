// command_sender.cpp - Command sender thread implementation

#include "command_sender.h"

#include "jon_shared_cmd.pb.h"
#include "jon_shared_cmd_day_camera.pb.h"
#include "jon_shared_cmd_heat_camera.pb.h"

#include <chrono>
#include <iostream>

namespace jettison
{

CommandSender::CommandSender (SharedTiming &timing, CommandQueue &queue,
                               WebSocketClient &ws_client,
                               std::atomic<bool> &running)
    : timing_ (timing), queue_ (queue), ws_client_ (ws_client),
      running_ (running)
{
}

void
CommandSender::run ()
{
  std::cout << "✓ Command sender thread started\n";

  while (running_.load ())
    {
      // Pop command with 1-second timeout
      auto cmd_opt = queue_.pop_with_timeout (std::chrono::seconds (1));

      if (!cmd_opt)
        {
          // Timeout - send ping
          auto ping_data = build_ping ();
          if (!ping_data.empty ())
            {
              ws_client_.send (ping_data.data (), ping_data.size ());
            }
          // Don't print ping messages (too verbose)
        }
      else
        {
          // User command
          auto cmd_data = build_command (*cmd_opt);

          // Only send if validation passed
          if (!cmd_data.empty ())
            {
              ws_client_.send (cmd_data.data (), cmd_data.size ());

              // Print command type
              switch (cmd_opt->type)
                {
                case CommandType::OPTICAL_ZOOM:
                  std::cout << "→ Sent optical zoom command (index: "
                            << cmd_opt->optical_zoom_index << ")\n";
                  break;
                case CommandType::DIGITAL_ZOOM:
                  std::cout << "→ Sent digital zoom command (level: "
                            << cmd_opt->digital_zoom_level << "x)\n";
                  break;
                default:
                  break;
                }
            }
        }
    }

  std::cout << "✓ Command sender thread stopped\n";
}

std::vector<uint8_t>
CommandSender::build_command (const Command &cmd)
{
  // Read current timing
  uint64_t frame_time_ns, system_time_ns;
  timing_.read (frame_time_ns, system_time_ns);

  // Get client wall-clock time
  auto now = std::chrono::system_clock::now ();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (
                now.time_since_epoch ())
                .count ();

  // Build protobuf command
  cmd::Root root;
  root.set_protocol_version (1);
  root.set_client_type (ser::JON_GUI_DATA_CLIENT_TYPE_LOCAL_NETWORK);
  root.set_client_app (ser::JON_GUI_DATA_CLIENT_APP_DESKTOP_NATIVE);

  // Timing fields (nanoseconds)
#ifdef CAMERA_TYPE_HEAT
  root.set_frame_time_heat (frame_time_ns);
  root.set_frame_time_day (0); // Stub
#else
  root.set_frame_time_day (frame_time_ns);
  root.set_frame_time_heat (0); // Stub
#endif

  root.set_state_time (system_time_ns);
  root.set_client_time_ms (static_cast<uint64_t> (ms));

  // Command-specific fields
  switch (cmd.type)
    {
    case CommandType::OPTICAL_ZOOM:
      {
        // Debug: print the actual value being set
        if (cmd.optical_zoom_index > 10 || cmd.optical_zoom_index < 0)
          {
            std::cerr << "[DEBUG] Building command with optical_zoom_index = "
                      << cmd.optical_zoom_index << " (testing validation)\n";
          }

#ifdef CAMERA_TYPE_HEAT
        auto *heat_cam = root.mutable_heat_camera ();
        auto *zoom = heat_cam->mutable_zoom ();
        auto *set_val = zoom->mutable_set_zoom_table_value ();
        set_val->set_value (cmd.optical_zoom_index);
#else
        auto *day_cam = root.mutable_day_camera ();
        auto *zoom = day_cam->mutable_zoom ();
        auto *set_val = zoom->mutable_set_zoom_table_value ();
        set_val->set_value (cmd.optical_zoom_index);
#endif
        break;
      }

    case CommandType::DIGITAL_ZOOM:
      {
#ifdef CAMERA_TYPE_HEAT
        auto *heat_cam = root.mutable_heat_camera ();
        auto *set_level = heat_cam->mutable_set_digital_zoom_level ();
        set_level->set_value (cmd.digital_zoom_level);
#else
        auto *day_cam = root.mutable_day_camera ();
        auto *set_level = day_cam->mutable_set_digital_zoom_level ();
        set_level->set_value (cmd.digital_zoom_level);
#endif
        break;
      }

    case CommandType::PING:
      // Ping handled separately
      break;
    }

  // Validate the command before sending
  auto validation_result = validator_.validate (root);

  // Print warnings if any
  for (const auto &warning : validation_result.warnings)
    {
      std::cerr << "⚠ Warning: " << warning << "\n";
    }

  if (!validation_result.is_valid)
    {
      // Flush stdout to ensure ordering
      std::cout << std::flush;

      std::cerr << "\n";
      std::cerr << "✗✗✗ VALIDATION FAILED - Command rejected ✗✗✗\n";
      std::cerr << "Validation errors (" << validation_result.errors.size ()
                << " total):\n";
      for (const auto &error : validation_result.errors)
        {
          std::cerr << "  - " << error << "\n";
        }
      std::cerr << "\n";

      // Print command as JSON for debugging
      std::cerr << "Rejected command (JSON):\n";
      std::cerr << json_converter_.to_json (root, true) << "\n";
      std::cerr << "\n";
      std::cerr << std::flush;

      // Return empty vector (don't send invalid command)
      return std::vector<uint8_t> ();
    }

  // Print validated command as JSON
  std::cout << "\nCommand (JSON):\n";
  std::cout << json_converter_.to_json (root, true) << "\n";

  // Serialize to bytes
  std::string serialized;
  root.SerializeToString (&serialized);

  return std::vector<uint8_t> (serialized.begin (), serialized.end ());
}

std::vector<uint8_t>
CommandSender::build_ping ()
{
  // Read current timing
  uint64_t frame_time_ns, system_time_ns;
  timing_.read (frame_time_ns, system_time_ns);

  // Get client wall-clock time
  auto now = std::chrono::system_clock::now ();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (
                now.time_since_epoch ())
                .count ();

  // Build ping command
  cmd::Root root;
  root.set_protocol_version (1);
  root.set_client_type (ser::JON_GUI_DATA_CLIENT_TYPE_LOCAL_NETWORK);
  root.set_client_app (ser::JON_GUI_DATA_CLIENT_APP_DESKTOP_NATIVE);

#ifdef CAMERA_TYPE_HEAT
  root.set_frame_time_heat (frame_time_ns);
  root.set_frame_time_day (0);
#else
  root.set_frame_time_day (frame_time_ns);
  root.set_frame_time_heat (0);
#endif

  root.set_state_time (system_time_ns);
  root.set_client_time_ms (static_cast<uint64_t> (ms));

  // Empty ping message
  root.mutable_ping ();

  // Validate the ping command
  auto validation_result = validator_.validate (root);

  // Print warnings if any
  for (const auto &warning : validation_result.warnings)
    {
      std::cerr << "⚠ Warning: " << warning << "\n";
    }

  if (!validation_result.is_valid)
    {
      std::cerr << "\n";
      std::cerr << "✗ VALIDATION FAILED - Ping rejected\n";
      std::cerr << "Validation errors:\n";
      for (const auto &error : validation_result.errors)
        {
          std::cerr << "  - " << error << "\n";
        }
      std::cerr << "\n";

      // Return empty vector
      return std::vector<uint8_t> ();
    }

  // Serialize
  std::string serialized;
  root.SerializeToString (&serialized);

  return std::vector<uint8_t> (serialized.begin (), serialized.end ());
}

} // namespace jettison
