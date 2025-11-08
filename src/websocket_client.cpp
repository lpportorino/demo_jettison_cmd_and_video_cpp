// websocket_client.cpp - WebSocket client implementation using IXWebSocket

#include "websocket_client.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <iostream>
#include <chrono>
#include <thread>

namespace jettison
{

// PIMPL implementation
struct WebSocketClient::Impl
{
  std::string hostname;
  int port;
  std::string path;

  ix::WebSocket websocket;
  MessageCallback message_callback;
  std::atomic<bool> connected{ false };

  Impl (const std::string &hostname_param, int port_param,
        const std::string &path_param)
      : hostname (hostname_param), port (port_param), path (path_param)
  {
    // Initialize IXWebSocket networking (idempotent)
    ix::initNetSystem ();

    // Build full URL: wss://hostname:port/path
    std::string url
        = "wss://" + hostname + ":" + std::to_string (port) + path;
    websocket.setUrl (url);

    // Configure TLS options (disable certificate verification for self-signed
    // certs)
    ix::SocketTLSOptions tlsOptions;
    tlsOptions.caFile = "NONE";  // Special value to disable cert verification
    websocket.setTLSOptions (tlsOptions);

    // Set custom Origin header (required by nginx)
    ix::WebSocketHttpHeaders headers;
    headers["Origin"] = "https://" + hostname;
    websocket.setExtraHeaders (headers);

    // Disable automatic reconnection
    websocket.disableAutomaticReconnection ();

    // Set message callback
    websocket.setOnMessageCallback (
        [this] (const ix::WebSocketMessagePtr &msg) {
          switch (msg->type)
            {
            case ix::WebSocketMessageType::Open:
              connected.store (true);
              std::cout << "✓ Connected to WebSocket\n";
              break;

            case ix::WebSocketMessageType::Close:
              connected.store (false);
              std::cerr << "✗ WebSocket closed: " << msg->closeInfo.reason
                        << "\n";
              break;

            case ix::WebSocketMessageType::Error:
              connected.store (false);
              std::cerr << "✗ WebSocket error: " << msg->errorInfo.reason
                        << "\n";
              break;

            case ix::WebSocketMessageType::Message:
              // Binary message received
              if (message_callback && !msg->str.empty ())
                {
                  message_callback (
                      reinterpret_cast<const uint8_t *> (msg->str.data ()),
                      msg->str.size ());
                }
              break;

            default:
              break;
            }
        });
  }

  ~Impl ()
  {
    disconnect ();
  }

  void
  disconnect ()
  {
    if (connected.load ())
      {
        websocket.stop ();
        connected.store (false);
      }
  }
};

WebSocketClient::WebSocketClient (const std::string &hostname, int port,
                                  const std::string &path)
    : impl_ (std::make_unique<Impl> (hostname, port, path))
{
}

WebSocketClient::~WebSocketClient ()
{
  disconnect ();
}

void
WebSocketClient::set_message_callback (MessageCallback callback)
{
  impl_->message_callback = std::move (callback);
}

bool
WebSocketClient::connect ()
{
  // Start the WebSocket connection (non-blocking)
  impl_->websocket.start ();

  // Wait for connection to establish (timeout: 5 seconds)
  for (int i = 0; i < 100; ++i)
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
      if (impl_->connected.load ())
        {
          return true;
        }
    }

  std::cerr << "✗ WebSocket connection timeout\n";
  return false;
}

bool
WebSocketClient::service (int timeout_ms)
{
  // IXWebSocket handles I/O in background threads
  // Just sleep for the timeout period
  if (timeout_ms > 0)
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (timeout_ms));
    }
  return impl_->connected.load ();
}

bool
WebSocketClient::send (const uint8_t *data, size_t len)
{
  if (!impl_->connected.load ())
    {
      return false;
    }

  // Send binary message
  std::string binary_data (reinterpret_cast<const char *> (data), len);
  auto result = impl_->websocket.sendBinary (binary_data);
  return result.success;
}

bool
WebSocketClient::is_connected () const
{
  return impl_->connected.load ();
}

void
WebSocketClient::disconnect ()
{
  impl_->disconnect ();
}

} // namespace jettison
