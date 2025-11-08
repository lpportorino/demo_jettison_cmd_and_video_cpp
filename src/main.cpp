// main.cpp - Jettison Command TX main application
// Heat/Day camera control with video streaming via named pipe

#include "command_queue.h"
#include "command_sender.h"
#include "keyboard_handler.h"
#include "shared_timing.h"
#include "video_receiver.h"
#include "video_writer.h"
#include "websocket_client.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>

// Global running flag for signal handlers
static std::atomic<bool> g_running{ true };

static void
signal_handler (int signum)
{
  std::cout << "\n→ Caught signal " << signum << ", shutting down...\n";
  g_running.store (false);
}

static void
print_help (const char *program_name)
{
  std::cout << "Jettison Command TX - Camera Control with Video Streaming\n";
  std::cout << "\n";
  std::cout << "Usage:\n";
  std::cout << "  " << program_name << " <hostname>\n";
  std::cout << "  " << program_name << " --help\n";
  std::cout << "\n";
  std::cout << "Arguments:\n";
  std::cout << "  <hostname>    Device hostname or IP (e.g., sych.local)\n";
  std::cout << "  --help        Show this help message\n";
  std::cout << "\n";
  std::cout << "Description:\n";
#ifdef CAMERA_TYPE_HEAT
  std::cout << "  Controls heat (thermal) camera zoom and streams video\n";
  std::cout << "  Video output: /tmp/jettison_heat.h264\n";
#else
  std::cout << "  Controls day (visible light) camera zoom and streams video\n";
  std::cout << "  Video output: /tmp/jettison_day.h264\n";
#endif
  std::cout << "\n";
  std::cout << "Keyboard Controls:\n";
  std::cout << "  1-5     Optical zoom positions (";
#ifdef CAMERA_TYPE_HEAT
  std::cout << "A-E";
#else
  std::cout << "I-V";
#endif
  std::cout << ")\n";
  std::cout << "  a-k     Digital zoom (1.0x - 6.0x, 0.5x steps)\n";
  std::cout << "  z/Z     Send invalid zoom (tests validation)\n";
  std::cout << "  q/Q     Quit\n";
  std::cout << "\n";
  std::cout << "Example:\n";
  std::cout << "  " << program_name << " sych.local\n";
  std::cout << "\n";
  std::cout << "View video stream (in separate terminal):\n";
#ifdef CAMERA_TYPE_HEAT
  std::cout << "  ffplay -fflags nobuffer -flags low_delay -framedrop "
               "/tmp/jettison_heat.h264\n";
#else
  std::cout << "  ffplay -fflags nobuffer -flags low_delay -framedrop "
               "/tmp/jettison_day.h264\n";
#endif
  std::cout << "\n";
}

int
main (int argc, char *argv[])
{
  // Parse arguments
  if (argc < 2 || std::string (argv[1]) == "--help"
      || std::string (argv[1]) == "-h")
    {
      print_help (argv[0]);
      return 0;
    }

  std::string hostname = argv[1];

  // Configuration
#ifdef CAMERA_TYPE_HEAT
  const char *CAMERA_NAME = "Heat";
  const char *VIDEO_PATH = "/ws/ws_rec_video_heat";  // Use _rec endpoint for H.264
  const char *PIPE_PATH = "/tmp/jettison_heat.h264";
#else
  const char *CAMERA_NAME = "Day";
  const char *VIDEO_PATH = "/ws/ws_rec_video_day";   // Use _rec endpoint for H.264
  const char *PIPE_PATH = "/tmp/jettison_day.h264";
#endif

  const char *CMD_PATH = "/ws/ws_cmd";
  const int WS_PORT = 443;

  std::cout << "\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "Jettison Command TX - " << CAMERA_NAME << " Camera\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "\n";

  // Setup signal handlers
  std::signal (SIGINT, signal_handler);
  std::signal (SIGTERM, signal_handler);

  // Shared state
  std::atomic<bool> running{ true };
  jettison::SharedTiming timing;
  jettison::CommandQueue cmd_queue;

  // Create components
  std::cout << "Initializing...\n";

  jettison::VideoWriter video_writer (PIPE_PATH);
  jettison::WebSocketClient ws_cmd (hostname, WS_PORT, CMD_PATH);
  jettison::VideoReceiver video_receiver (hostname, WS_PORT, VIDEO_PATH,
                                          timing, video_writer, running);
  jettison::CommandSender cmd_sender (timing, cmd_queue, ws_cmd, running);
  jettison::KeyboardHandler keyboard (cmd_queue, running);

  std::cout << "\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "WebSocket Endpoints\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "  Command:  wss://" << hostname << ":" << WS_PORT << CMD_PATH
            << "\n";
  std::cout << "  Video:    wss://" << hostname << ":" << WS_PORT << VIDEO_PATH
            << "\n";
  std::cout << "  Origin:   https://" << hostname << "\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "\n";

  // Connect to command WebSocket
  std::cout << "Connecting to command endpoint...\n";

  if (!ws_cmd.connect ())
    {
      std::cerr << "✗ Failed to connect to command endpoint\n";
      return 1;
    }

  std::cout << "✓ Connected to command endpoint\n";

  // Start threads
  std::cout << "\n";
  std::cout << "Starting threads...\n";

  std::thread video_thread ([&] () { video_receiver.run (); });

  std::thread cmd_sender_thread ([&] () { cmd_sender.run (); });

  // Print startup info
  std::cout << "\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "✓ Connected to " << hostname << "\n";
  std::cout << "✓ Video pipe: " << PIPE_PATH << "\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "\n";
  std::cout << "View video stream in another terminal:\n";
  std::cout << "\n";
  std::cout << "FFplay (recommended):\n";
  std::cout << "  ffplay -fflags nobuffer -flags low_delay -framedrop "
            << PIPE_PATH << "\n";
  std::cout << "\n";
  std::cout << "MPV:\n";
  std::cout << "  mpv --no-cache --untimed " << PIPE_PATH << "\n";
  std::cout << "\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "\n";
  std::cout << "WARNING: First frames may not be keyframes.\n";
  std::cout << "         You may see decoding errors until next I-frame.\n";
  std::cout << "         This is normal - FFplay handles this gracefully.\n";
  std::cout << "\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "\n";

  jettison::KeyboardHandler::print_help ();

  std::cout << "\n";
  std::cout << "Press ENTER to start command mode...\n";
  std::cin.get ();

  std::cout << "\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "Command mode active - awaiting keyboard input\n";
  std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << "\n";

  // Main event loop
  while (running.load () && g_running.load ())
    {
      // Service command WebSocket
      ws_cmd.service (50); // 50ms timeout

      // Check keyboard input (non-blocking)
      keyboard.check_input ();
    }

  // Cleanup
  std::cout << "\n";
  std::cout << "Shutting down...\n";

  running.store (false);

  // Wait for threads
  if (video_thread.joinable ())
    {
      video_thread.join ();
    }

  if (cmd_sender_thread.joinable ())
    {
      cmd_sender_thread.join ();
    }

  std::cout << "✓ Shutdown complete\n";
  std::cout << "\n";

  return 0;
}
