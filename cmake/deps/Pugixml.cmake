include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS)
    nexus_print_dependency_mode(pugixml "FetchContent v1.14")

    set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(PUGIXML_BUILD_SHARED_AND_STATIC_LIBS OFF CACHE BOOL "" FORCE)
    set(PUGIXML_INSTALL OFF CACHE BOOL "" FORCE)

    # pugixml is compiled directly into nexus_common (see src/common/CMakeLists.txt).
    # We only FetchContent it here to obtain the source; the upstream CMake build is
    # intentionally skipped via a non-existent SOURCE_SUBDIR.
    FetchContent_Declare(
        pugixml
        GIT_REPOSITORY https://github.com/zeux/pugixml.git
        GIT_TAG db78afc2b7d8f043b4bc6b185635d949ea2ed2a8
        GIT_CONFIG ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
        SOURCE_SUBDIR cmake/nexuskit-no-add-subdirectory
    )

    FetchContent_MakeAvailable(pugixml)
else()
    nexus_print_dependency_mode(pugixml "find_package")
    find_package(pugixml REQUIRED CONFIG)
endif()
