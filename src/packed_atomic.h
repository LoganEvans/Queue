#pragma once

#include <cstdint>
#include <tuple>
#include <utility>
#include <variant>

namespace theta {

template <typename... Types>
class PackedAtomic {
 public:
  template <int index>
  using ResultType =
      typename std::tuple_element<index, std::tuple<Types...>>::type;

  static constexpr size_t kNumTypes
      = std::tuple_size<std::tuple<Types...>>::value;

 private:
  template <int index>
  static constexpr size_t offset() {
    using prev_type = ResultType<index - 1>;
    using curr_type = ResultType<index>;

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
    constexpr size_t min_bytes
        = offset<kNumTypes - 1>() + sizeof(ResultType<kNumTypes - 1>);

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

  using possible_containing_types
      = std::variant<int8_t, int16_t, int32_t, int64_t, __int128_t>;

  static constexpr int containing_types_index() {
    if constexpr (bytes() == 1) {
      return 0;
    } else if constexpr (bytes() == 2) {
      return 1;
    } else if constexpr (bytes() == 4) {
      return 2;
    } else if constexpr (bytes() == 8) {
      return 3;
    }
    return 4;
  }

  template <typename FirstType, typename... RestTypes>
  void variadic_ctor_helper(FirstType firstArg, RestTypes... restArgs) {
    constexpr size_t numRestTypes
        = std::tuple_size<std::tuple<RestTypes...>>::value;
    constexpr size_t index = kNumTypes - numRestTypes - 1;
    set<index>(firstArg);
    variadic_ctor_helper(restArgs...);
  }

  void variadic_ctor_helper() {}

 public:
  using ContainingType = typename std::variant_alternative<
      PackedAtomic<Types...>::containing_types_index(),
      possible_containing_types>::type;

  using AtomicContainingType = std::atomic<ContainingType>;

  PackedAtomic() : container_(0) {}

  PackedAtomic(Types... args) { variadic_ctor_helper(args...); }

  template <typename... Types2>
  PackedAtomic(const PackedAtomic<Types2...>& other)
      : container_(other.container_) {}

  template <typename... Types2>
  PackedAtomic(const PackedAtomic<Types2...>& other,
               std::memory_order mem_order) {
    other.fetch(mem_order);
    container_ = other.container_;
  }

  template <typename... Types2>
  PackedAtomic<Types2...> operator=(const PackedAtomic<Types2...>& other) {
    container_ = other.container_;
    return *this;
  }

  template <int index>
  auto get() const {
    ResultType<index> result;
    memcpy(&result, as_nonatomic<index>(), sizeof(ResultType<index>));
    return result;
  }

  template <int index>
  ResultType<index>* as_nonatomic() const {
    return reinterpret_cast<ResultType<index>*>(
        reinterpret_cast<char*>(const_cast<ContainingType*>(&container_))
        + PackedAtomic<Types...>::offset<index>());
  }

  template <int index>
  std::atomic<ResultType<index>>* as_atomic() const {
    return reinterpret_cast<std::atomic<ResultType<index>>*>(
        reinterpret_cast<char*>(const_cast<ContainingType*>(&container_))
        + PackedAtomic<Types...>::offset<index>());
  }

  ContainingType container() const { return container_; }

  AtomicContainingType* container_as_atomic() {
    return reinterpret_cast<std::atomic<ContainingType>*>(&container_);
  }

  template <int index>
  auto get_atomic(std::memory_order mem_order
                  = std::memory_order::acquire) const {
    return as_atomic<index>()->load(mem_order);
  }

  template <int index>
  void set(ResultType<index> value) {
    memcpy(as_nonatomic<index>(), &value, sizeof(value));
  }

  template <int index>
  void set_atomic(ResultType<index> value,
                  std::memory_order mem_order = std::memory_order::release) {
    as_atomic<index>()->store(value, mem_order);
  }

  template <int index>
  ResultType<index> fetch_add(ResultType<index> val,
                              std::memory_order mem_order
                              = std::memory_order::acq_rel) {
    return as_atomic<index>()->fetch_add(mem_order);
  }

  void fetch(std::memory_order mem_order = std::memory_order::acquire) {
    container_ = container_as_atomic()->load(mem_order);
  }

  void flush(std::memory_order mem_order = std::memory_order::release) {
    container_as_atomic()->store(container_, mem_order);
  }

  alignas(sizeof(ContainingType)) ContainingType container_{0};
};

}  // namespace theta
