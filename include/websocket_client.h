// websocket_client.h - WebSocket client using IXWebSocket
// Supports both video streaming (receive) and commands (send)

#ifndef JETTISON_WEBSOCKET_CLIENT_H
#define JETTISON_WEBSOCKET_CLIENT_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace jettison
{

// Callback for received binary messages
using MessageCallback = std::function<void (const uint8_t *data, size_t len)>;

// WebSocket client for WSS connections
class WebSocketClient
{
public:
  // Constructor
  // hostname: Device hostname (e.g., "sych.local")
  // port: Port number (443 for HTTPS/WSS)
  // path: WebSocket path (e.g., "/ws/ws_cmd" or "/ws/ws_video_heat")
  WebSocketClient (const std::string &hostname, int port,
                   const std::string &path);

  ~WebSocketClient ();

  // Non-copyable, non-movable
  WebSocketClient (const WebSocketClient &) = delete;
  WebSocketClient &operator= (const WebSocketClient &) = delete;
  WebSocketClient (WebSocketClient &&) = delete;
  WebSocketClient &operator= (WebSocketClient &&) = delete;

  // Set callback for received messages
  void set_message_callback (MessageCallback callback);

  // Connect to WebSocket endpoint
  // Returns true if connection succeeds, false otherwise
  bool connect ();

  // Service the WebSocket (process events)
  // timeout_ms: Timeout in milliseconds (0 = non-blocking)
  // Returns true if still connected, false if disconnected
  bool service (int timeout_ms);

  // Send binary message
  // Returns true if message queued successfully, false otherwise
  bool send (const uint8_t *data, size_t len);

  // Check if connected
  bool is_connected () const;

  // Disconnect
  void disconnect ();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace jettison

#endif // JETTISON_WEBSOCKET_CLIENT_H
