include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS)
    nexus_print_dependency_mode(Catch2 "FetchContent v3.5.4")

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG abb467ecd60fae9a727afca033c1eb5d20af2c12
        GIT_CONFIG ${NEXUS_GIT_CONFIG_ARGS}
    )

    FetchContent_MakeAvailable(Catch2)
else()
    nexus_print_dependency_mode(Catch2 "find_package")
    find_package(Catch2 3 REQUIRED CONFIG)
endif()
