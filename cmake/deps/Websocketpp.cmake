include_guard(GLOBAL)

include(FetchContent)
include(Asio)

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
        if(POLICY CMP0169)
            cmake_policy(PUSH)
            cmake_policy(SET CMP0169 OLD)
        endif()
        FetchContent_Populate(websocketpp_source)
        if(POLICY CMP0169)
            cmake_policy(POP)
        endif()
    endif()

    if(NOT TARGET websocketpp::websocketpp)
        add_library(nexus_websocketpp INTERFACE)
        add_library(websocketpp::websocketpp ALIAS nexus_websocketpp)
        target_include_directories(nexus_websocketpp SYSTEM INTERFACE "${websocketpp_source_SOURCE_DIR}")
        target_compile_definitions(nexus_websocketpp INTERFACE
            ASIO_STANDALONE
            _WEBSOCKETPP_CPP11_STL_
            $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0601>
        )
        target_compile_options(nexus_websocketpp INTERFACE
            $<$<CXX_COMPILER_ID:MSVC>:/external:W0 /external:anglebrackets>
        )
        target_link_libraries(nexus_websocketpp INTERFACE asio::asio)
    endif()
else()
    nexus_print_dependency_mode(websocketpp "find_package")
    find_package(websocketpp REQUIRED CONFIG)
endif()
