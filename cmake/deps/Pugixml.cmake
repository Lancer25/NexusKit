include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS)
    nexus_print_dependency_mode(pugixml "FetchContent v1.14")

    set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(PUGIXML_BUILD_SHARED_AND_STATIC_LIBS OFF CACHE BOOL "" FORCE)
    set(PUGIXML_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        pugixml
        GIT_REPOSITORY https://github.com/zeux/pugixml.git
        GIT_TAG db78afc2b7d8f043b4bc6b185635d949ea2ed2a8
        GIT_CONFIG ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
        SOURCE_SUBDIR cmake/nexuskit-no-add-subdirectory
    )

    FetchContent_MakeAvailable(pugixml)

    if(NOT TARGET pugixml)
        add_library(pugixml STATIC EXCLUDE_FROM_ALL
            "${pugixml_SOURCE_DIR}/src/pugixml.cpp"
        )

        add_library(pugixml::pugixml ALIAS pugixml)

        target_include_directories(pugixml
            PUBLIC
                "${pugixml_SOURCE_DIR}/src"
        )

        target_compile_features(pugixml PUBLIC cxx_std_17)
        set_target_properties(pugixml PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
else()
    nexus_print_dependency_mode(pugixml "find_package")
    find_package(pugixml REQUIRED CONFIG)
endif()
