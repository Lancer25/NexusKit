include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS)
    nexus_print_dependency_mode(asio "FetchContent 1.30.2")

    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG 42a93679dc4c8c5caf3d3082542f1bfa2438271d
        SOURCE_SUBDIR asio
        GIT_CONFIG ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
    )

    FetchContent_MakeAvailable(asio)
else()
    nexus_print_dependency_mode(asio "find_package")
    find_package(asio REQUIRED CONFIG)
endif()
