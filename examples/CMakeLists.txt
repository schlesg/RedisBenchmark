cmake_minimum_required(VERSION 3.5)
project(RedisBenchmark )

set(EXAMPLES
    # async_pubsub2.cpp
    BenchmarkPublisher.cpp
    BenchmarkSubscriber.cpp
    # async_pubsub.cpp
    # async_set_get2.cpp
    # async_set_get.cpp
    # async_timeout.cpp
    # sync_pipeline.cpp
    # sync_set_get.cpp
    # benchmark.cpp
    # sync_benchmark.cpp
    # sync_timeout.cpp
)
find_package(nlohmann_json REQUIRED)

foreach(EXAMPLE ${EXAMPLES})
  get_filename_component(EXECUTABLE ${EXAMPLE} NAME_WE)
  add_executable(${EXECUTABLE} ${EXAMPLE})
  target_link_libraries(${EXECUTABLE}
      RedisClient
      ${Boost_PROGRAM_OPTIONS_LIBRARY}
      ${Boost_DATE_TIME_LIBRARY}
       nlohmann_json
       )
#   target_link_libraries(${EXECUTABLE} RedisClient nlohmann_json)

endforeach()

# target_link_libraries(benchmark
#     RedisClient
#     ${Boost_PROGRAM_OPTIONS_LIBRARY}
# )

# target_link_libraries(sync_benchmark
#     RedisClient
#     ${Boost_PROGRAM_OPTIONS_LIBRARY}
# )
