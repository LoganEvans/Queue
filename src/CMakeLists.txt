add_library(
  packed-atomic INTERFACE
  packed_atomic.h
)
target_include_directories(packed-atomic INTERFACE .)
target_link_libraries(packed-atomic INTERFACE atomic)

add_library(
  types INTERFACE
  types.h
)
target_include_directories(types INTERFACE .)

add_library(
  mpmc-queue INTERFACE
  mpmc_queue.h
)
target_include_directories(mpmc-queue INTERFACE .)
target_link_libraries(mpmc-queue INTERFACE atomic)

add_library(
  mpsc-queue INTERFACE
  mpsc_queue.h
)
target_include_directories(mpsc-queue INTERFACE .)
target_link_libraries(mpmc-queue INTERFACE atomic)
