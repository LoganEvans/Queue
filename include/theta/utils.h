#pragma once

#include <atomic>
#include <concepts>
#include <type_traits>

namespace theta {

template <typename T>
concept FlushableType = alignof(T) >= sizeof(T) && sizeof(T) <= 16;

template <FlushableType T>
inline void flush_atomic(T* t,
                         std::memory_order mem_order
                         = std::memory_order::release) {
  using AtomicType = std::conditional_t<
      sizeof(T) <= 1,
      std::atomic<uint8_t>,
      std::conditional_t<
          sizeof(T) <= 2,
          std::atomic<uint16_t>,
          std::conditional_t<sizeof(T) <= 4,
                             std::atomic<uint32_t>,
                             std::conditional_t<sizeof(T) <= 8,
                                                std::atomic<uint64_t>,
                                                std::atomic<__int128>>>>>;

  std::atomic_store_explicit(reinterpret_cast<AtomicType*>(t),
                             *reinterpret_cast<AtomicType*>(t),
                             mem_order);
}

template <FlushableType T>
inline void fetch_atomic(T* t,
                         std::memory_order mem_order
                         = std::memory_order::acquire) {
  using AtomicType = std::conditional_t<
      sizeof(T) <= 1,
      std::atomic<uint8_t>,
      std::conditional_t<
          sizeof(T) <= 2,
          std::atomic<uint16_t>,
          std::conditional_t<sizeof(T) <= 4,
                             std::atomic<uint32_t>,
                             std::conditional_t<sizeof(T) <= 8,
                                                std::atomic<uint64_t>,
                                                std::atomic<__int128>>>>>;

  auto res
      = std::atomic_load_explicit(reinterpret_cast<AtomicType*>(t), mem_order);
  memcpy(t, &res, sizeof(T));
}

}  // namespace theta
