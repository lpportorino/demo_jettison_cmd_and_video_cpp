// video_writer.cpp - Named pipe video writer implementation

#include "video_writer.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace jettison
{

VideoWriter::VideoWriter (const std::string &pipe_path)
    : pipe_path_ (pipe_path), fd_ (-1), opened_ (false)
{
  // Remove existing pipe if present
  unlink (pipe_path_.c_str ());

  // Create named pipe (FIFO) with read/write permissions for all
  if (mkfifo (pipe_path_.c_str (), 0666) != 0)
    {
      std::cerr << "✗ Failed to create named pipe: " << pipe_path_
                << " (" << strerror (errno) << ")\n";
      return;
    }

  std::cout << "✓ Video pipe created: " << pipe_path_ << "\n";
  std::cout << "  Frames will be dropped until viewer connects\n";
  std::cout << "  WARNING: First frames after connection may not be "
               "keyframes\n";

  // Try to open pipe in non-blocking mode
  // This will fail with ENXIO if no reader, which is OK
  fd_ = open (pipe_path_.c_str (), O_WRONLY | O_NONBLOCK);
  if (fd_ >= 0)
    {
      // Set pipe buffer size to 5MB to handle large frames
      // Note: May be limited by /proc/sys/fs/pipe-max-size
      constexpr int PIPE_BUFFER_SIZE = 5 * 1024 * 1024; // 5 MB
      if (fcntl (fd_, F_SETPIPE_SZ, PIPE_BUFFER_SIZE) == -1)
        {
          std::cerr << "⚠ Failed to set pipe buffer size to "
                    << PIPE_BUFFER_SIZE << " bytes: " << strerror (errno)
                    << "\n";
          std::cerr << "  Continuing with default buffer size\n";
        }
      else
        {
          std::cout << "✓ Pipe buffer size set to 5 MB\n";
        }

      opened_.store (true);
      std::cout << "✓ Video viewer already connected\n";
    }
}

VideoWriter::~VideoWriter ()
{
  if (fd_ >= 0)
    {
      close (fd_);
    }

  // Remove named pipe
  unlink (pipe_path_.c_str ());
}

bool
VideoWriter::write_frame (const uint8_t *data, size_t len)
{
  // Try to open pipe if not already opened
  if (!opened_.load () && fd_ < 0)
    {
      fd_ = open (pipe_path_.c_str (), O_WRONLY | O_NONBLOCK);
      if (fd_ >= 0)
        {
          // Set pipe buffer size to 5MB to handle large frames
          constexpr int PIPE_BUFFER_SIZE = 5 * 1024 * 1024; // 5 MB
          if (fcntl (fd_, F_SETPIPE_SZ, PIPE_BUFFER_SIZE) == -1)
            {
              std::cerr << "⚠ Failed to set pipe buffer size to "
                        << PIPE_BUFFER_SIZE << " bytes: " << strerror (errno)
                        << "\n";
            }
          else
            {
              std::cout << "✓ Pipe buffer size set to 5 MB\n";
            }

          opened_.store (true);
          std::cout << "✓ Video viewer connected to " << pipe_path_ << "\n";
          std::cout << "  WARNING: First frames may not be keyframes\n";
        }
    }

  if (fd_ < 0)
    {
      // No reader - silently drop frame
      return false;
    }

  // Write frame to pipe (non-blocking)
  ssize_t written = write (fd_, data, len);

  if (written < 0)
    {
      if (errno == EPIPE)
        {
          // Reader disconnected (SIGPIPE)
          close (fd_);
          fd_ = -1;
          opened_.store (false);
          std::cout << "⚠ Video viewer disconnected from " << pipe_path_
                    << "\n";
        }
      else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          // Pipe buffer full - drop frame
          // This happens when reader is too slow
          return false;
        }
      else
        {
          // Other error
          std::cerr << "✗ Video pipe write error: " << strerror (errno)
                    << "\n";
        }
      return false;
    }

  // Check if full frame was written
  if (written != static_cast<ssize_t> (len))
    {
      std::cerr << "⚠ Partial frame write: " << written << "/" << len
                << " bytes\n";
      return false;
    }

  return true;
}

} // namespace jettison
