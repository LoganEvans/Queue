#include <gtest/gtest.h>

#include "packed_atomic.h"

TEST(PackedAtomicTest, foo) {
  PackedAtomic<int, double> foo;

  EXPECT_EQ(foo.bytes(), 16);
}

TEST(PackedAtomicTest, offsets) {
  PackedAtomic<int, double> foo;

  EXPECT_EQ(foo.offset<0>(), 0);
  EXPECT_EQ(foo.offset<1>(), 8);
}

TEST(PackedAtomicTest, size) {
  EXPECT_EQ(sizeof(PackedAtomic<int16_t, int16_t>), 4);
  EXPECT_EQ(sizeof(PackedAtomic<int8_t, int16_t>), 4);
  EXPECT_EQ(sizeof(PackedAtomic<int16_t, int32_t>), 8);
  EXPECT_EQ(sizeof(PackedAtomic<int32_t, double>), 16);
}

