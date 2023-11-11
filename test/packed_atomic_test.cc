#include <gtest/gtest.h>

#include "packed_atomic.h"

namespace theta {

TEST(PackedAtomicTest, size) {
  EXPECT_EQ(sizeof(PackedAtomic<int16_t, int16_t>), 4);
  EXPECT_EQ(sizeof(PackedAtomic<int8_t, int16_t>), 4);
  EXPECT_EQ(sizeof(PackedAtomic<int16_t, int32_t>), 8);
  EXPECT_EQ(sizeof(PackedAtomic<int32_t, double>), 16);
}

TEST(PackedAtomicTest, get_set) {
  PackedAtomic<int, double> foo;

  for (int i = -100; i < 100; i++) {
    foo.set<0>(i);
    EXPECT_EQ(foo.get<0>(), i);

    foo.set<1>(i * 2.5);
    EXPECT_EQ(foo.get<1>(), i * 2.5);
  }
}

TEST(PackedAtomicTest, get_set_atomic) {
  PackedAtomic<int, double> foo;
  PackedAtomic<uint64_t> bar;

  for (int i = -100; i < 100; i++) {
    foo.set_atomic<0>(i);
    EXPECT_EQ(foo.get_atomic<0>(), i);

    foo.set_atomic<1>(i * 2.5);
    EXPECT_EQ(foo.get_atomic<1>(), i * 2.5);

    bar.set_atomic<0>(i);
    EXPECT_EQ(bar.get_atomic<0>(), i);
  }
}

TEST(PackedAtomicTest, fetch_flush) {
  PackedAtomic<int, double> foo;
  foo.set<0>(1);
  foo.set<1>(2.2);
  foo.fetch();
  foo.flush();
}

TEST(PackedAtomicTest, copy_ctor) {
  PackedAtomic<int, double> foo;
  foo.set<0>(1);
  foo.set<1>(2.2);

  auto bar{foo};
  EXPECT_EQ(bar.get<0>(), 1);
  EXPECT_EQ(bar.get<1>(), 2.2);
}

TEST(PackedAtomicTest, variadic_ctor) {
    auto foo = PackedAtomic{1, 2, 3.0};
    EXPECT_EQ(foo.kNumTypes, 3);
    EXPECT_EQ(foo.get<0>(), 1);
    EXPECT_EQ(foo.get<1>(), 2);
    EXPECT_EQ(foo.get<2>(), 3.0);

    PackedAtomic<int8_t, int8_t, int8_t, int8_t, int8_t> bar{1, 2, 3, 4, 5};
    EXPECT_EQ(bar.get<0>(), 1);
    EXPECT_EQ(bar.get<1>(), 2);
    EXPECT_EQ(bar.get<2>(), 3);
    EXPECT_EQ(bar.get<3>(), 4);
    EXPECT_EQ(bar.get<4>(), 5);
}

TEST(PackedAtomicTest, assignment) {
  PackedAtomic<int, double> foo, bar;
  foo.set<0>(1);
  foo.set<1>(2.2);

  bar = foo;
  EXPECT_EQ(bar.get<0>(), 1);
  EXPECT_EQ(bar.get<1>(), 2.2);
}

}  // namespace theta
