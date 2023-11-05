#pragma once

#include <cstdint>
#include <tuple>
#include <utility>
#include <variant>

// struct Tag {
//   static_assert((kBufferSize & (kBufferSize - 1)) == 0, "");
//
//   // An kIncrement value of
//   //   kIncrement = 1 + hardware_destructive_interference_size / sizeof(T)
//   // would make it so that adjacent values will always be on separate cache
//   // lines. However, benchmarks don't show that this attempt to avoiding
//   // false sharing helps.
//   static constexpr uint64_t kIncrement = 1;
//   // static constexpr uint64_t kIncrement =
//   //     1 + hardware_destructive_interference_size / sizeof(T);
//   static constexpr uint64_t kBufferWrapDelta = kBufferSize * kIncrement;
//   static constexpr uint64_t kBufferSizeMask = kBufferSize - 1;
//   static constexpr uint64_t kConsumerFlag = (1ULL << 63);
//   static constexpr uint64_t kWaitingFlag = (1ULL << 62);
//
//   uint64_t raw;
//
//   Tag(uint64_t raw) : raw(raw) {}
//   Tag() : Tag(0) {}
//
//   Tag& operator++() {
//     raw += kIncrement;
//     return *this;
//   }
//
//   Tag& operator++(int) {
//     raw += kIncrement;
//     return *this;
//   }
//
//   Tag& operator--() {
//     raw -= kIncrement;
//     return *this;
//   }
//
//   Tag& operator--(int) {
//     raw -= kIncrement;
//     return *this;
//   }
//
//   auto operator<=>(const Tag&) const = default;
//
//   std::string DebugString() const {
//     return "Tag<" + std::string(is_producer() ? "P" : "C")
//          + (is_waiting() ? std::string("|W") : "") + ">{"
//          + std::to_string(value()) + "@" + std::to_string(to_index()) + "}";
//   }
//
//   uint64_t value() const { return (raw << 2) >> 2; }
//
//   Tag prev_paired_tag() const {
//     if (is_consumer()) {
//       return Tag{(raw ^ kConsumerFlag) & ~kWaitingFlag};
//     } else {
//       return Tag{((raw - kBufferWrapDelta) ^ kConsumerFlag) & ~kWaitingFlag};
//     }
//   }
//
//   bool is_paired(Tag observed_tag) const {
//     return prev_paired_tag().raw == (observed_tag.raw & ~kWaitingFlag);
//   }
//
//   bool is_producer() const { return (raw & kConsumerFlag) == 0; }
//
//   void mark_as_producer() { raw &= ~kConsumerFlag; }
//
//   bool is_consumer() const { return (raw & kConsumerFlag) > 0; }
//
//   void mark_as_consumer() { raw |= kConsumerFlag; }
//
//   void mark_as_waiting() { raw |= kWaitingFlag; }
//
//   bool is_waiting() const { return (raw & kWaitingFlag) > 0; }
//
//   void clear_waiting_flag() { raw &= ~kWaitingFlag; }
//
//   int to_index() const { return raw & kBufferSizeMask; }
// };
//   static_assert(sizeof(Tag) == sizeof(uint64_t), "");
//
//
// union Index {
//   struct {
//     Tag tag;
//   };
//   struct {
//     std::atomic<Tag> tag_atomic;
//   };
//   struct {
//     std::atomic<uint64_t> tag_raw_atomic;
//   };
//
//   Index(Tag tag) : tag(tag) {}
//   Index() : Index(/*tag=*/0) {}
//
//   std::string DebugString() const {
//     return "Index{tag=" + tag.DebugString() + "}";
//   }
// };
// static_assert(sizeof(Index) == sizeof(Tag), "");

template <typename... Types>
struct PackedAtomic {
  template <size_t index>
  static constexpr size_t offset() {
    using prev_type =
        typename std::tuple_element<index - 1, std::tuple<Types...>>::type;
    using curr_type =
        typename std::tuple_element<index, std::tuple<Types...>>::type;

    constexpr size_t prev_end = offset<index - 1>() + sizeof(prev_type);
    constexpr size_t remainder = prev_end % sizeof(curr_type);
    if constexpr (remainder == 0) {
      return prev_end;
    }

    return prev_end + sizeof(curr_type) - remainder;
  }

  template <>
  static constexpr size_t offset<0>() {
    return 0;
  }

  static constexpr size_t bytes() {
    constexpr size_t num_types = std::tuple_size<std::tuple<Types...>>::value;

    constexpr size_t min_bytes
        = offset<std::tuple_size<std::tuple<Types...>>::value - 1>()
        + sizeof(typename std::tuple_element<num_types - 1,
                                             std::tuple<Types...>>::type);

    if constexpr (min_bytes <= 1) {
      return 1;
    } else if constexpr (min_bytes <= 2) {
      return 2;
    } else if constexpr (min_bytes <= 4) {
      return 4;
    } else if constexpr (min_bytes <= 8) {
      return 8;
    } else if constexpr (min_bytes <= 16) {
      return 16;
    } else {
      static_assert(min_bytes <= 16, "Cannot pack types into PackedAtomic");
    }

    return min_bytes;
  }

  using possible_containing_types = std::variant<std::atomic<int8_t>,
               std::atomic<int16_t>,
               std::atomic<int32_t>,
               std::atomic<int64_t>,
               std::atomic<__int128_t>>;

  static constexpr int containing_types_index() {
    constexpr auto b = PackedAtomic<Types...>::bytes();
    if constexpr (b == 1) {
      return 0;
    } else if constexpr (b == 2) {
      return 1;
    } else if constexpr (b == 4) {
      return 2;
    } else if constexpr (b == 8) {
      return 3;
    }
    return 4;
  }

  using ContainingType = typename std::variant_alternative<
      PackedAtomic<Types...>::containing_types_index(),
      possible_containing_types>::type;

  ContainingType container{0};
};

