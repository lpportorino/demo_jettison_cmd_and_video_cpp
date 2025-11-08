// shared_timing.h - Lock-free atomic timing storage
// Shared between video receiver thread and command sender thread

#ifndef JETTISON_SHARED_TIMING_H
#define JETTISON_SHARED_TIMING_H

#include <atomic>
#include <cstdint>

namespace jettison
{

// Atomic storage for frame timing data
// Written by video receiver thread, read by command sender thread
// Slight inconsistency between frame_time_ns and system_time_ns is acceptable
class SharedTiming
{
public:
  SharedTiming () = default;
  ~SharedTiming () = default;

  // Non-copyable, non-movable
  SharedTiming (const SharedTiming &) = delete;
  SharedTiming &operator= (const SharedTiming &) = delete;
  SharedTiming (SharedTiming &&) = delete;
  SharedTiming &operator= (SharedTiming &&) = delete;

  // Update timing data (called by video receiver thread)
  void update (uint64_t frame_time_ns, uint64_t system_time_ns) noexcept;

  // Read timing data (called by command sender thread)
  void read (uint64_t &frame_time_ns, uint64_t &system_time_ns) const noexcept;

private:
  std::atomic<uint64_t> frame_time_ns_{ 0 };
  std::atomic<uint64_t> system_time_ns_{ 0 };
};

} // namespace jettison

#endif // JETTISON_SHARED_TIMING_H
