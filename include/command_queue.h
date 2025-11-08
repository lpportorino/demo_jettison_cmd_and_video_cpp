// command_queue.h - Thread-safe command queue with timeout
// Blocking queue for commands from keyboard to command sender thread

#ifndef JETTISON_COMMAND_QUEUE_H
#define JETTISON_COMMAND_QUEUE_H

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>

namespace jettison
{

// Command types
enum class CommandType : uint8_t
{
  OPTICAL_ZOOM, // Optical zoom position (0-4)
  DIGITAL_ZOOM, // Digital zoom level (1.0-6.0)
  PING,         // Ping/keepalive
};

// Command structure
struct Command
{
  CommandType type;

  // Union for command-specific data
  union
  {
    int optical_zoom_index;     // 0-4 (positions A-E or I-V), stored as int for validation
    float digital_zoom_level;   // 1.0-6.0
  };

  // Constructor for optical zoom
  static Command optical_zoom (int index);

  // Constructor for digital zoom
  static Command digital_zoom (float level);

  // Constructor for ping
  static Command ping ();
};

// Thread-safe blocking queue with timeout support
class CommandQueue
{
public:
  CommandQueue () = default;
  ~CommandQueue () = default;

  // Non-copyable, non-movable
  CommandQueue (const CommandQueue &) = delete;
  CommandQueue &operator= (const CommandQueue &) = delete;
  CommandQueue (CommandQueue &&) = delete;
  CommandQueue &operator= (CommandQueue &&) = delete;

  // Push command (called by keyboard handler in main thread)
  void push (const Command &cmd);

  // Pop command with timeout (called by command sender thread)
  // Returns std::nullopt if timeout expires (1 second)
  std::optional<Command>
  pop_with_timeout (std::chrono::milliseconds timeout);

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::queue<Command> queue_;
};

} // namespace jettison

#endif // JETTISON_COMMAND_QUEUE_H
