// video_writer.h - Named pipe video writer for local H.264 streaming
// Writes raw H.264 AVCC data to FIFO (named pipe) for viewing with ffplay/vlc/mpv

#ifndef JETTISON_VIDEO_WRITER_H
#define JETTISON_VIDEO_WRITER_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

namespace jettison
{

// Video writer using named pipes (FIFO)
// Non-blocking writes - drops frames if no reader or pipe full
class VideoWriter
{
public:
  // Constructor
  // pipe_path: Path to named pipe (e.g., "/tmp/jettison_heat.h264")
  explicit VideoWriter (const std::string &pipe_path);

  ~VideoWriter ();

  // Non-copyable, non-movable
  VideoWriter (const VideoWriter &) = delete;
  VideoWriter &operator= (const VideoWriter &) = delete;
  VideoWriter (VideoWriter &&) = delete;
  VideoWriter &operator= (VideoWriter &&) = delete;

  // Write H.264 AVCC frame to named pipe
  // data: H.264 AVCC data (length-prefixed NAL units)
  // len: Size of data in bytes
  // Returns true if frame written successfully, false if dropped
  //
  // NOTE: Frames may be dropped if:
  // - No reader attached to pipe
  // - Pipe buffer is full (reader too slow)
  // - After dropping frames, first frame may not be a keyframe
  bool write_frame (const uint8_t *data, size_t len);

  // Get pipe path
  std::string get_path () const { return pipe_path_; }

  // Check if reader is connected
  bool has_reader () const { return opened_.load (); }

private:
  std::string pipe_path_;
  int fd_;
  std::atomic<bool> opened_;
};

} // namespace jettison

#endif // JETTISON_VIDEO_WRITER_H
