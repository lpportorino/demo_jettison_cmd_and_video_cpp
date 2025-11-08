// keyboard_handler.h - Non-blocking keyboard input handler
// Reads keyboard input and pushes commands to queue

#ifndef JETTISON_KEYBOARD_HANDLER_H
#define JETTISON_KEYBOARD_HANDLER_H

#include "command_queue.h"

#include <atomic>

namespace jettison
{

// Keyboard handler - non-blocking keyboard input
// Runs in main thread alongside WebSocket event loop
class KeyboardHandler
{
public:
  // Constructor
  // queue: Command queue to push commands
  // running: Shared running flag (for exit on 'q')
  KeyboardHandler (CommandQueue &queue, std::atomic<bool> &running);

  ~KeyboardHandler ();

  // Non-copyable, non-movable
  KeyboardHandler (const KeyboardHandler &) = delete;
  KeyboardHandler &operator= (const KeyboardHandler &) = delete;
  KeyboardHandler (KeyboardHandler &&) = delete;
  KeyboardHandler &operator= (KeyboardHandler &&) = delete;

  // Check for keyboard input (non-blocking)
  // Called from main loop
  void check_input ();

  // Print help message
  static void print_help ();

private:
  // Handle a key press
  void handle_key (char key);

  // Set terminal to non-blocking mode
  void set_nonblocking_mode ();

  // Restore terminal to normal mode
  void restore_terminal_mode ();

  CommandQueue &queue_;
  std::atomic<bool> &running_;

  // Terminal state (for restoration)
  void *old_termios_ = nullptr; // Actually struct termios*
};

} // namespace jettison

#endif // JETTISON_KEYBOARD_HANDLER_H
