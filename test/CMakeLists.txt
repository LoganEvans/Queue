enable_testing()

add_executable(queue-test queue_test.cc)
target_link_libraries(queue-test mpmc-queue mpsc-queue GTest::gmock
                      GTest::gtest_main)

add_executable(utils-test utils_test.cc)
target_link_libraries(utils-test PUBLIC utils GTest::gmock GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(queue-test)
gtest_discover_tests(utils-test)
