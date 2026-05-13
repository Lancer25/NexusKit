include(Catch2)

add_executable(nexus_core_tests
    core/status_result_tests.cpp
)

target_link_libraries(nexus_core_tests
    PRIVATE
        nexus::core
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(nexus_core_tests)
