// shared_timing.cpp - Lock-free atomic timing storage implementation

#include "shared_timing.h"

namespace jettison
{

void
SharedTiming::update (uint64_t frame_time_ns,
                      uint64_t system_time_ns) noexcept
{
  // Store with relaxed memory ordering (lock-free)
  // Slight inconsistency between the two values is acceptable
  frame_time_ns_.store (frame_time_ns, std::memory_order_relaxed);
  system_time_ns_.store (system_time_ns, std::memory_order_relaxed);
}

void
SharedTiming::read (uint64_t &frame_time_ns,
                    uint64_t &system_time_ns) const noexcept
{
  // Load with relaxed memory ordering (lock-free)
  frame_time_ns = frame_time_ns_.load (std::memory_order_relaxed);
  system_time_ns = system_time_ns_.load (std::memory_order_relaxed);
}

} // namespace jettison
