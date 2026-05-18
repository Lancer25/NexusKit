include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS AND NEXUS_BUILD_HIDAPI)
    nexus_print_dependency_mode(hidapi "FetchContent 0.14.0")

    set(HIDAPI_BUILD_HIDTEST OFF CACHE BOOL "" FORCE)
    set(HIDAPI_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(HIDAPI_INSTALL_TARGETS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        hidapi
        GIT_REPOSITORY https://github.com/libusb/hidapi.git
        GIT_TAG 73d292a8f4d18f7fb532b9829b9a26ca8d864419
        GIT_CONFIG ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
    )

    if(DEFINED CACHE{CMAKE_WARN_DEPRECATED})
        set(_nexus_hidapi_had_warn_deprecated_cache TRUE)
        set(_nexus_hidapi_warn_deprecated_cache "$CACHE{CMAKE_WARN_DEPRECATED}")
    else()
        set(_nexus_hidapi_had_warn_deprecated_cache FALSE)
    endif()
    if(DEFINED CACHE{CMAKE_SKIP_INSTALL_RULES})
        set(_nexus_hidapi_had_skip_install_rules_cache TRUE)
        set(_nexus_hidapi_skip_install_rules_cache "$CACHE{CMAKE_SKIP_INSTALL_RULES}")
    else()
        set(_nexus_hidapi_had_skip_install_rules_cache FALSE)
    endif()
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
    set(CMAKE_SKIP_INSTALL_RULES TRUE CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(hidapi)
    if(_nexus_hidapi_had_skip_install_rules_cache)
        set(CMAKE_SKIP_INSTALL_RULES "${_nexus_hidapi_skip_install_rules_cache}" CACHE BOOL "" FORCE)
    else()
        unset(CMAKE_SKIP_INSTALL_RULES CACHE)
    endif()
    if(_nexus_hidapi_had_warn_deprecated_cache)
        set(CMAKE_WARN_DEPRECATED "${_nexus_hidapi_warn_deprecated_cache}" CACHE BOOL "" FORCE)
    else()
        unset(CMAKE_WARN_DEPRECATED CACHE)
    endif()
    unset(_nexus_hidapi_had_warn_deprecated_cache)
    unset(_nexus_hidapi_warn_deprecated_cache)
    unset(_nexus_hidapi_had_skip_install_rules_cache)
    unset(_nexus_hidapi_skip_install_rules_cache)

    if(MSVC AND TARGET hidapi_winapi)
        target_link_options(hidapi_winapi PRIVATE /IGNORE:4006)
        set_target_properties(hidapi_winapi PROPERTIES STATIC_LIBRARY_OPTIONS "/IGNORE:4006")
    endif()
else()
    nexus_print_dependency_mode(hidapi "find_package")
    find_package(hidapi REQUIRED CONFIG)
endif()
