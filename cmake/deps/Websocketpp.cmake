include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS)
    nexus_print_dependency_mode(websocketpp "FetchContent 0.8.2")

    FetchContent_Declare(
        websocketpp_source
        GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
        GIT_TAG 0e4241727c199e208ceb41134e840ba9968cf181
        GIT_CONFIG ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
    )

    FetchContent_GetProperties(websocketpp_source)
    if(NOT websocketpp_source_POPULATED)
        FetchContent_Populate(websocketpp_source)
    endif()

    if(NOT TARGET websocketpp::websocketpp)
        add_library(nexus_websocketpp INTERFACE)
        add_library(websocketpp::websocketpp ALIAS nexus_websocketpp)
        target_include_directories(nexus_websocketpp INTERFACE "${websocketpp_source_SOURCE_DIR}")
    endif()
else()
    nexus_print_dependency_mode(websocketpp "find_package")
    find_package(websocketpp REQUIRED CONFIG)
endif()
