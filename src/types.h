#pragma once

#include "packed_atomic.h"

namespace theta {

struct Tag {
  static_assert((kBufferSize & (kBufferSize - 1)) == 0, "");

  // An kIncrement value of
  //   kIncrement = 1 + hardware_destructive_interference_size / sizeof(T)
  // would make it so that adjacent values will always be on separate cache
  // lines. However, benchmarks don't show that this attempt to avoiding
  // false sharing helps.
  static constexpr uint64_t kIncrement = 1;
  // static constexpr uint64_t kIncrement =
  //     1 + hardware_destructive_interference_size / sizeof(T);
  static constexpr uint64_t kBufferWrapDelta = kBufferSize * kIncrement;
  static constexpr uint64_t kBufferSizeMask = kBufferSize - 1;
  static constexpr uint64_t kConsumerFlag = (1ULL << 63);
  static constexpr uint64_t kWaitingFlag = (1ULL << 62);

  packed_atomic<uint64_t> raw;

  Tag(uint64_t raw) : raw(raw) {}
  Tag() : Tag(0) {}

  Tag& operator++() {
    raw.set<0>(raw.get<0> + kIncrement);
    return *this;
  }

  Tag& operator++(int) {
    raw.set<0>(raw.get<0> + kIncrement);
    return *this;
  }

  Tag& operator--() {
    raw.set<0>(raw.get<0> - kIncrement);
    return *this;
  }

  Tag& operator--(int) {
    raw.set<0>(raw.get<0> - kIncrement);
    return *this;
  }

  auto operator<=>(const Tag&) const = default;

  std::string DebugString() const {
    return "Tag<" + std::string(is_producer() ? "P" : "C")
         + (is_waiting() ? std::string("|W") : "") + ">{"
         + std::to_string(value()) + "@" + std::to_string(to_index()) + "}";
  }

  uint64_t value() const { return (raw.get<0>() << 2) >> 2; }

  Tag prev_paired_tag() const {
    if (is_consumer()) {
      return Tag{(raw.get<0>() ^ kConsumerFlag) & ~kWaitingFlag};
    } else {
      return Tag{((raw.get<0>() - kBufferWrapDelta) ^ kConsumerFlag)
                 & ~kWaitingFlag};
    }
  }

  bool is_paired(Tag observed_tag) const {
    return prev_paired_tag().raw.get<0>()
        == (observed_tag.raw.get<0>() & ~kWaitingFlag);
  }

  bool is_producer() const { return (raw.get<0>() & kConsumerFlag) == 0; }

  void mark_as_producer() { raw.get<0>() &= ~kConsumerFlag; }

  bool is_consumer() const { return (raw.get<0>() & kConsumerFlag) > 0; }

  void mark_as_consumer() { raw.get<0>() |= kConsumerFlag; }

  void mark_as_waiting() { raw.get<0>() |= kWaitingFlag; }

  bool is_waiting() const { return (raw.get<0>() & kWaitingFlag) > 0; }

  void clear_waiting_flag() { raw.get<0>() &= ~kWaitingFlag; }

  int to_index() const { return raw.get<0>() & kBufferSizeMask; }
};
static_assert(sizeof(Tag) == sizeof(uint64_t), "");

}  // namespace theta
