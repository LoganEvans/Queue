#include <gtest/gtest.h>
#include <theta/utils.h>

namespace theta {

TEST(UtilsTest, flush) {
  struct Foo {
    int a;
  } foo;

  flush_atomic(&foo);
  fetch_atomic(&foo);
}

}  // namespace theta
