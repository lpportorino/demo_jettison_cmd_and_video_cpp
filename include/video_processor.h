// video_processor.h - Video frame parsing and processing
// Parses 28-byte custom header + H.264 AVCC payload

#ifndef JETTISON_VIDEO_PROCESSOR_H
#define JETTISON_VIDEO_PROCESSOR_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace jettison
{

// Video frame header structure (28 bytes)
// All fields are little-endian
struct VideoFrameHeader
{
  uint32_t unknown1;       // Offset 0-4
  uint32_t sequence;       // Offset 4-8 (frame sequence number)
  uint64_t duration_us;    // Offset 8-16 (frame duration in microseconds)
  uint64_t system_time_us; // Offset 16-24 (CLOCK_MONOTONIC in microseconds)
  uint16_t unknown2;       // Offset 24-26
  uint16_t unknown3;       // Offset 26-28
} __attribute__ ((packed));

static_assert (sizeof (VideoFrameHeader) == 28,
               "VideoFrameHeader must be 28 bytes");

// Parsed video frame data
struct ParsedVideoFrame
{
  uint32_t sequence;         // Frame sequence number
  uint64_t duration_ns;      // Frame duration in nanoseconds
  uint64_t system_time_ns;   // System monotonic time in nanoseconds
  const uint8_t *h264_data;  // Pointer to H.264 AVCC data (not owned)
  size_t h264_size;          // Size of H.264 data in bytes
};

// Video frame processor
class VideoProcessor
{
public:
  VideoProcessor () = default;
  ~VideoProcessor () = default;

  // Non-copyable, non-movable
  VideoProcessor (const VideoProcessor &) = delete;
  VideoProcessor &operator= (const VideoProcessor &) = delete;
  VideoProcessor (VideoProcessor &&) = delete;
  VideoProcessor &operator= (VideoProcessor &&) = delete;

  // Parse video frame from WebSocket binary data
  // Returns true if parsing succeeded, false otherwise
  // The h264_data pointer in result points into the input data buffer
  bool parse_frame (const uint8_t *data, size_t size,
                    ParsedVideoFrame &result) const;

private:
  // Validate frame header
  bool validate_header (const VideoFrameHeader *header, size_t total_size) const;
};

} // namespace jettison

#endif // JETTISON_VIDEO_PROCESSOR_H
