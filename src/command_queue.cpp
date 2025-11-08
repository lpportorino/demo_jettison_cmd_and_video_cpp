// command_queue.cpp - Thread-safe command queue implementation

#include "command_queue.h"

namespace jettison
{

Command
Command::optical_zoom (int index)
{
  Command cmd;
  cmd.type = CommandType::OPTICAL_ZOOM;
  cmd.optical_zoom_index = index;
  return cmd;
}

Command
Command::digital_zoom (float level)
{
  Command cmd;
  cmd.type = CommandType::DIGITAL_ZOOM;
  cmd.digital_zoom_level = level;
  return cmd;
}

Command
Command::ping ()
{
  Command cmd;
  cmd.type = CommandType::PING;
  return cmd;
}

void
CommandQueue::push (const Command &cmd)
{
  std::lock_guard<std::mutex> lock (mutex_);
  queue_.push (cmd);
  cv_.notify_one ();
}

std::optional<Command>
CommandQueue::pop_with_timeout (std::chrono::milliseconds timeout)
{
  std::unique_lock<std::mutex> lock (mutex_);

  // Wait for queue to have elements or timeout
  if (!cv_.wait_for (lock, timeout, [this] { return !queue_.empty (); }))
    {
      // Timeout expired, queue is empty
      return std::nullopt;
    }

  // Queue has elements, pop one
  Command cmd = queue_.front ();
  queue_.pop ();
  return cmd;
}

} // namespace jettison
