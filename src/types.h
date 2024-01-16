#pragma once

#include "packed_atomic.h"

namespace theta {

template <size_t kBufferSize>
struct Tag {
  using RawType = uint64_t;
  using ContainingType = PackedAtomic<RawType>::ContainingType;
  using AtomicContainingType = PackedAtomic<RawType>::AtomicContainingType;

  static_assert((kBufferSize & (kBufferSize - 1)) == 0, "");

  // An kIncrement value of
  //   kIncrement = 1 + hardware_destructive_interference_size / sizeof(T)
  // would make it so that adjacent values will always be on separate cache
  // lines. However, benchmarks don't show that this attempt to avoiding
  // false sharing helps.
  static constexpr RawType kIncrement = 1;
  // static constexpr RawType kIncrement =
  //     1 + hardware_destructive_interference_size / sizeof(T);
  static constexpr RawType kBufferWrapDelta = kBufferSize * kIncrement;
  static constexpr RawType kBufferSizeMask = kBufferSize - 1;
  static constexpr RawType kConsumerFlag = (1ULL << 63);
  static constexpr RawType kWaitingFlag = (1ULL << 62);

  PackedAtomic<RawType> raw;

  Tag(RawType raw) : raw(raw) {}
  Tag() : Tag(0) {}

  Tag& operator++() {
    raw.set<0>(raw.get<0>() + kIncrement);
    return *this;
  }

  Tag& operator++(int) {
    raw.set<0>(raw.get<0>() + kIncrement);
    return *this;
  }

  Tag& operator--() {
    raw.set<0>(raw.get<0>() - kIncrement);
    return *this;
  }

  Tag& operator--(int) {
    raw.set<0>(raw.get<0>() - kIncrement);
    return *this;
  }

  auto operator<=>(const Tag&) const = default;

  std::string DebugString() const {
    return "Tag<" + std::string(is_producer() ? "P" : "C")
         + (is_waiting() ? std::string("|W") : "") + ">{"
         + std::to_string(value()) + "@" + std::to_string(to_index()) + "}";
  }

  RawType value() const { return (raw.get<0>() << 2) >> 2; }
  RawType value_atomic() const { return (raw.get_atomic<0>() << 2) >> 2; }

  Tag prev_paired_tag() const {
    if (is_consumer()) {
      return Tag{(raw.get<0>() ^ kConsumerFlag) & ~kWaitingFlag};
    } else {
      return Tag{((raw.get<0>() - kBufferWrapDelta) ^ kConsumerFlag)
                 & ~kWaitingFlag};
    }
  }

  bool is_paired(Tag observed_tag) const {
    return prev_paired_tag().raw.template get<0>()
        == (observed_tag.raw.template get<0>() & ~kWaitingFlag);
  }

  bool is_producer() const { return (raw.get<0>() & kConsumerFlag) == 0; }

  void mark_as_producer() { raw.set<0>(raw.get<0>() & ~kConsumerFlag); }

  bool is_consumer() const { return (raw.get<0>() & kConsumerFlag) > 0; }

  void mark_as_consumer() { raw.set<0>(raw.get<0>() | kConsumerFlag); }

  void mark_as_waiting() { raw.set<0>(raw.get<0>() | kWaitingFlag); }

  bool is_waiting() const { return (raw.get<0>() & kWaitingFlag) > 0; }

  void clear_waiting_flag() { raw.set<0>(raw.get<0>() & ~kWaitingFlag); }

  int to_index() const { return raw.get<0>() & kBufferSizeMask; }

  Tag<kBufferSize> reserve() {
    return Tag<kBufferSize>{
        static_cast<RawType>(raw.container_as_atomic()->fetch_add(
            kIncrement, std::memory_order::acq_rel))};
  }

  std::optional<Tag<kBufferSize>> try_reserve(RawType max_allowed_value) {
    const Tag<kBufferSize> limit{max_allowed_value};

    Tag<kBufferSize>::ContainingType expected{
        static_cast<Tag<kBufferSize>::ContainingType>(limit.value())};
    Tag<kBufferSize> desired{limit};
    desired++;

    while (!raw.container_as_atomic()->compare_exchange_weak(
        expected,
        desired.raw.container(),
        std::memory_order::release,
        std::memory_order::relaxed)) {
      desired.raw.template set<0>(expected);
      desired++;
      if (desired.value() > limit.value()) {
        return {};
      }
    }

    return {Tag<kBufferSize>{static_cast<Tag<kBufferSize>::RawType>(expected)}};
  }
};
static_assert(sizeof(Tag<128>) == sizeof(Tag<128>::RawType), "");

}  // namespace theta
