enable_testing()

add_executable(
  queue_test
  queue_test.cc
)
target_link_libraries(
  queue_test
  mpmc-queue
  mpsc-queue
  GTest::gmock
  GTest::gtest_main
)

add_executable(
  packed_atomic_test
  packed_atomic_test.cc
)
target_link_libraries(
  packed_atomic_test
  packed-atomic
  GTest::gmock
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(queue_test)
