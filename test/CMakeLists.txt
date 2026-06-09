# Unit tests for the pure protocol helpers, retry policy, and secret/TLS config
# resolution. All three suites are transport-agnostic (no live server, no gRPC),
# so they build and run whether or not the gRPC transport is enabled.

include(FetchContent)

if(NOT TARGET gtest)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(neuriplo-kserve-client-tests
    ${CMAKE_CURRENT_SOURCE_DIR}/test_KserveProtocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_KserveRetry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_KserveSecurity.cpp
)

target_link_libraries(neuriplo-kserve-client-tests PRIVATE
    neuriplo-kserve-client
    nlohmann_json::nlohmann_json
    gtest
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(neuriplo-kserve-client-tests)
