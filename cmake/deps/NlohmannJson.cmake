include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS)
    nexus_print_dependency_mode(nlohmann_json "FetchContent v3.11.3")

    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install OFF CACHE INTERNAL "")

    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03
        GIT_CONFIG ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
    )

    FetchContent_MakeAvailable(nlohmann_json)
else()
    nexus_print_dependency_mode(nlohmann_json "find_package")
    find_package(nlohmann_json REQUIRED CONFIG)
endif()
