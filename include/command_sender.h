// command_sender.h - Build and send protobuf commands
// Runs in dedicated thread, pops commands from queue and sends via WebSocket

#ifndef JETTISON_COMMAND_SENDER_H
#define JETTISON_COMMAND_SENDER_H

#include "command_queue.h"
#include "json_converter.h"
#include "proto_validator.h"
#include "shared_timing.h"
#include "websocket_client.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace jettison
{

// Command sender - builds and sends protobuf commands
// Runs in dedicated thread
class CommandSender
{
public:
  // Constructor
  // timing: Shared timing data (read frame/system time)
  // queue: Command queue to pop from
  // ws_client: WebSocket client to send commands
  // running: Shared running flag (for exit)
  CommandSender (SharedTiming &timing, CommandQueue &queue,
                 WebSocketClient &ws_client, std::atomic<bool> &running);

  ~CommandSender () = default;

  // Non-copyable, non-movable
  CommandSender (const CommandSender &) = delete;
  CommandSender &operator= (const CommandSender &) = delete;
  CommandSender (CommandSender &&) = delete;
  CommandSender &operator= (CommandSender &&) = delete;

  // Run command sender thread (blocking)
  // Pops commands from queue with 1-second timeout
  // Sends ping on timeout
  void run ();

  // Build command protobuf from Command struct
  // Returns serialized protobuf bytes
  std::vector<uint8_t> build_command (const Command &cmd);

private:
  // Build ping command
  std::vector<uint8_t> build_ping ();

  SharedTiming &timing_;
  CommandQueue &queue_;
  WebSocketClient &ws_client_;
  std::atomic<bool> &running_;
  ProtoValidator validator_;
  JsonConverter json_converter_;
};

} // namespace jettison

#endif // JETTISON_COMMAND_SENDER_H
