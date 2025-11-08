// rtsp_server.cpp - GStreamer RTSP server implementation

#include "rtsp_server.h"

#include <cstring>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>

namespace jettison
{

// PIMPL implementation
struct RtspServer::Impl
{
  std::string stream_name;
  int port;

  GstRTSPServer *server = nullptr;
  GstRTSPMountPoints *mounts = nullptr;
  GstRTSPMediaFactory *factory = nullptr;
  GstElement *appsrc = nullptr;

  guint attach_id = 0;
  bool started = false;

  // Frame counter for PTS
  uint64_t frame_count = 0;

  // GStreamer setup
  static void media_configure (GstRTSPMediaFactory *factory,
                                GstRTSPMedia *media, gpointer user_data);
  static void need_data (GstElement *appsrc, guint unused, gpointer user_data);
  static void enough_data (GstElement *appsrc, gpointer user_data);
};

RtspServer::RtspServer (const std::string &stream_name, int port)
    : impl_ (std::make_unique<Impl> ())
{
  impl_->stream_name = stream_name;
  impl_->port = port;

  // Initialize GStreamer
  gst_init (nullptr, nullptr);
}

RtspServer::~RtspServer ()
{
  stop ();
}

bool
RtspServer::start ()
{
  if (impl_->started)
    {
      return true;
    }

  // Create RTSP server
  impl_->server = gst_rtsp_server_new ();
  if (!impl_->server)
    {
      std::cerr << "✗ Failed to create RTSP server\n";
      return false;
    }

  // Set port
  gchar port_str[16];
  g_snprintf (port_str, sizeof (port_str), "%d", impl_->port);
  g_object_set (impl_->server, "service", port_str, nullptr);

  // Get mount points
  impl_->mounts = gst_rtsp_server_get_mount_points (impl_->server);

  // Create factory for stream
  impl_->factory = gst_rtsp_media_factory_new ();

  // Pipeline: appsrc → h264parse → rtph264pay → RTSP clients
  // h264parse converts AVCC (length-prefixed) to byte-stream (Annex B)
  std::string pipeline
      = "( appsrc name=src is-live=true format=time ! "
        "h264parse ! "
        "video/x-h264,stream-format=byte-stream,alignment=au ! "
        "rtph264pay name=pay0 pt=96 config-interval=1 )";

  gst_rtsp_media_factory_set_launch (impl_->factory, pipeline.c_str ());
  gst_rtsp_media_factory_set_shared (impl_->factory, TRUE);

  // Connect media-configure signal to set up appsrc
  g_signal_connect (impl_->factory, "media-configure",
                    G_CALLBACK (Impl::media_configure), impl_.get ());

  // Add factory to mounts
  std::string mount_point = "/" + impl_->stream_name;
  gst_rtsp_mount_points_add_factory (impl_->mounts, mount_point.c_str (),
                                     impl_->factory);

  g_object_unref (impl_->mounts);

  // Attach server to default main context
  impl_->attach_id = gst_rtsp_server_attach (impl_->server, nullptr);
  if (impl_->attach_id == 0)
    {
      std::cerr << "✗ Failed to attach RTSP server to main context\n";
      g_object_unref (impl_->server);
      impl_->server = nullptr;
      return false;
    }

  impl_->started = true;
  return true;
}

void
RtspServer::stop ()
{
  if (!impl_->started)
    {
      return;
    }

  if (impl_->appsrc)
    {
      gst_app_src_end_of_stream (GST_APP_SRC (impl_->appsrc));
      impl_->appsrc = nullptr;
    }

  if (impl_->server)
    {
      g_object_unref (impl_->server);
      impl_->server = nullptr;
    }

  impl_->started = false;
}

bool
RtspServer::push_frame (const uint8_t *data, size_t size, uint64_t pts_ns)
{
  if (!impl_->started || !impl_->appsrc)
    {
      return false;
    }

  // Create GStreamer buffer
  GstBuffer *buffer = gst_buffer_new_allocate (nullptr, size, nullptr);
  if (!buffer)
    {
      return false;
    }

  // Fill buffer with H.264 AVCC data
  GstMapInfo map;
  if (!gst_buffer_map (buffer, &map, GST_MAP_WRITE))
    {
      gst_buffer_unref (buffer);
      return false;
    }

  memcpy (map.data, data, size);
  gst_buffer_unmap (buffer, &map);

  // Set PTS and duration
  GST_BUFFER_PTS (buffer) = pts_ns;
  GST_BUFFER_DURATION (buffer) = 33333333; // ~30 FPS (33.333ms)

  // Push buffer to appsrc
  GstFlowReturn ret
      = gst_app_src_push_buffer (GST_APP_SRC (impl_->appsrc), buffer);

  if (ret != GST_FLOW_OK)
    {
      std::cerr << "✗ Failed to push buffer to appsrc: " << ret << "\n";
      return false;
    }

  impl_->frame_count++;
  return true;
}

std::string
RtspServer::get_url () const
{
  return "rtsp://localhost:" + std::to_string (impl_->port) + "/"
         + impl_->stream_name;
}

// GStreamer callbacks
void
RtspServer::Impl::media_configure (GstRTSPMediaFactory *factory,
                                    GstRTSPMedia *media, gpointer user_data)
{
  auto *impl = static_cast<Impl *> (user_data);

  // Get pipeline element
  GstElement *pipeline = gst_rtsp_media_get_element (media);

  // Get appsrc from pipeline
  impl->appsrc = gst_bin_get_by_name (GST_BIN (pipeline), "src");
  if (!impl->appsrc)
    {
      std::cerr << "✗ Failed to get appsrc from pipeline\n";
      gst_object_unref (pipeline);
      return;
    }

  // Configure appsrc
  g_object_set (G_OBJECT (impl->appsrc), "is-live", TRUE, "format",
                GST_FORMAT_TIME, "caps",
                gst_caps_new_simple ("video/x-h264", "stream-format",
                                     G_TYPE_STRING, "avc", "alignment",
                                     G_TYPE_STRING, "au", nullptr),
                nullptr);

  // Connect signals
  g_signal_connect (impl->appsrc, "need-data", G_CALLBACK (need_data),
                    user_data);
  g_signal_connect (impl->appsrc, "enough-data", G_CALLBACK (enough_data),
                    user_data);

  gst_object_unref (pipeline);
}

void
RtspServer::Impl::need_data (GstElement *appsrc, guint unused,
                              gpointer user_data)
{
  // Appsrc needs more data
  // We push frames on demand from video receiver thread
}

void
RtspServer::Impl::enough_data (GstElement *appsrc, gpointer user_data)
{
  // Appsrc has enough data buffered
  // We continue pushing frames anyway (live stream)
}

} // namespace jettison
