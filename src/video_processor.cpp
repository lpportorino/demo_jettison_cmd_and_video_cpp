// video_processor.cpp - Video frame parsing implementation

#include "video_processor.h"

#include <cstring>

namespace jettison
{

bool
VideoProcessor::parse_frame (const uint8_t *data, size_t size,
                              ParsedVideoFrame &result) const
{
  // Check minimum size (28-byte header + at least some data)
  if (size < sizeof (VideoFrameHeader) + 1)
    {
      return false;
    }

  // Parse header (little-endian, packed struct)
  const auto *header = reinterpret_cast<const VideoFrameHeader *> (data);

  // Validate header
  if (!validate_header (header, size))
    {
      return false;
    }

  // Extract timing data (convert microseconds to nanoseconds)
  result.sequence = header->sequence;
  result.duration_ns = header->duration_us * 1000ULL;
  result.system_time_ns = header->system_time_us * 1000ULL;

  // Extract H.264 AVCC data (offset 28+)
  result.h264_data = data + sizeof (VideoFrameHeader);
  result.h264_size = size - sizeof (VideoFrameHeader);

  return true;
}

bool
VideoProcessor::validate_header (const VideoFrameHeader *header,
                                  size_t total_size) const
{
  // Check duration is reasonable (should be ~33,333 µs for 30 FPS)
  // Allow range: 20,000 to 50,000 µs (20 to 50 FPS)
  if (header->duration_us < 20000 || header->duration_us > 50000)
    {
      return false;
    }

  // Check system time is non-zero and reasonable
  // System monotonic time should be > 0 and < 10 years in microseconds
  constexpr uint64_t TEN_YEARS_US = 10ULL * 365 * 24 * 3600 * 1000000;
  if (header->system_time_us == 0 || header->system_time_us > TEN_YEARS_US)
    {
      return false;
    }

  // Check that we have H.264 data after the header
  if (total_size <= sizeof (VideoFrameHeader))
    {
      return false;
    }

  return true;
}

} // namespace jettison
