include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS AND NEXUS_BUILD_LIBUVC)
    include(LibUSB)

    nexus_print_dependency_mode(libuvc "FetchContent v0.0.7")

    FetchContent_Declare(
        libuvc
        GIT_REPOSITORY https://github.com/libuvc/libuvc.git
        GIT_TAG        v0.0.7
        GIT_CONFIG     ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
    )

    set(CMAKE_BUILD_TARGET "Static" CACHE STRING "" FORCE)
    set(BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
    set(ENABLE_UVC_DEBUGGING OFF CACHE BOOL "" FORCE)

    if(DEFINED CACHE{CMAKE_WARN_DEPRECATED})
        set(_nexus_libuvc_had_warn_deprecated_cache TRUE)
        set(_nexus_libuvc_warn_deprecated_cache "$CACHE{CMAKE_WARN_DEPRECATED}")
    else()
        set(_nexus_libuvc_had_warn_deprecated_cache FALSE)
    endif()
    if(DEFINED CACHE{CMAKE_SKIP_INSTALL_RULES})
        set(_nexus_libuvc_had_skip_install_rules_cache TRUE)
        set(_nexus_libuvc_skip_install_rules_cache "$CACHE{CMAKE_SKIP_INSTALL_RULES}")
    else()
        set(_nexus_libuvc_had_skip_install_rules_cache FALSE)
    endif()
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
    set(CMAKE_SKIP_INSTALL_RULES TRUE CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(libuvc)

    # Provide POSIX compatibility shims for MSVC (libuvc uses <sys/time.h>, <pthread.h>)
    if(MSVC)
        set(_nexus_uvc_compat_dir "${CMAKE_BINARY_DIR}/_deps/libuvc-compat")
        file(MAKE_DIRECTORY "${_nexus_uvc_compat_dir}/sys")

        # sys/time.h — struct timeval, struct timespec, and CLOCK_* constants
        file(WRITE "${_nexus_uvc_compat_dir}/sys/time.h"
            "#pragma once\n"
            "\n"
            "#include <time.h>\n"
            "\n"
            "#ifdef _MSC_VER\n"
            "#include <winsock2.h> /* struct timeval */\n"
            "#else\n"
            "struct timeval {\n"
            "    long tv_sec;\n"
            "    long tv_usec;\n"
            "};\n"
            "#endif\n"
            "\n"
            "struct timezone {\n"
            "    int tz_minuteswest;\n"
            "    int tz_dsttime;\n"
            "};\n"
            "\n"
            "#define CLOCK_REALTIME  0\n"
            "#define CLOCK_MONOTONIC 1\n"
        )

        # pthread.h — maps pthread + clock_gettime to Windows primitives
        file(WRITE "${_nexus_uvc_compat_dir}/pthread.h"
            "#pragma once\n"
            "\n"
            "#ifndef _PTHREAD_H_WIN32_\n"
            "#define _PTHREAD_H_WIN32_\n"
            "\n"
            "#include <windows.h>\n"
            "#undef ARRAYSIZE /* let libuvc_internal.h define its own version */\n"
            "#include <errno.h>\n"
            "#include <sys/time.h>\n"
            "\n"
            "#ifdef __cplusplus\n"
            "extern \"C\" {\n"
            "#endif\n"
            "\n"
            "/* ------- clock_gettime (Windows implementation) ------- */\n"
            "\n"
            "static __inline int clock_gettime(int clk_id, struct timespec *tp)\n"
            "{\n"
            "    if (clk_id == CLOCK_MONOTONIC) {\n"
            "        LARGE_INTEGER freq, counter;\n"
            "        QueryPerformanceFrequency(&freq);\n"
            "        QueryPerformanceCounter(&counter);\n"
            "        tp->tv_sec = (time_t)(counter.QuadPart / freq.QuadPart);\n"
            "        tp->tv_nsec = (long)((counter.QuadPart % freq.QuadPart) * 1000000000LL / freq.QuadPart);\n"
            "    } else {\n"
            "        FILETIME ft;\n"
            "        GetSystemTimePreciseAsFileTime(&ft);\n"
            "        ULARGE_INTEGER t;\n"
            "        t.LowPart = ft.dwLowDateTime;\n"
            "        t.HighPart = ft.dwHighDateTime;\n"
            "        t.QuadPart -= 116444736000000000ULL;\n"
            "        tp->tv_sec = (time_t)(t.QuadPart / 10000000ULL);\n"
            "        tp->tv_nsec = (long)((t.QuadPart % 10000000ULL) * 100);\n"
            "    }\n"
            "    return 0;\n"
            "}\n"
            "\n"
            "/* ------- pthread types ------- */\n"
            "\n"
            "typedef CRITICAL_SECTION pthread_mutex_t;\n"
            "typedef struct { int _unused; } pthread_mutexattr_t;\n"
            "\n"
            "typedef CONDITION_VARIABLE pthread_cond_t;\n"
            "typedef struct { int _unused; } pthread_condattr_t;\n"
            "\n"
            "typedef HANDLE pthread_t;\n"
            "typedef struct { int _unused; } pthread_attr_t;\n"
            "\n"
            "/* ------- mutex ------- */\n"
            "\n"
            "static __inline int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a)\n"
            "{ (void)a; InitializeCriticalSection(m); return 0; }\n"
            "\n"
            "static __inline int pthread_mutex_destroy(pthread_mutex_t *m)\n"
            "{ DeleteCriticalSection(m); return 0; }\n"
            "\n"
            "static __inline int pthread_mutex_lock(pthread_mutex_t *m)\n"
            "{ EnterCriticalSection(m); return 0; }\n"
            "\n"
            "static __inline int pthread_mutex_unlock(pthread_mutex_t *m)\n"
            "{ LeaveCriticalSection(m); return 0; }\n"
            "\n"
            "/* ------- condition variable ------- */\n"
            "\n"
            "static __inline int pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *a)\n"
            "{ (void)a; InitializeConditionVariable(c); return 0; }\n"
            "\n"
            "static __inline int pthread_cond_destroy(pthread_cond_t *c)\n"
            "{ (void)c; return 0; }\n"
            "\n"
            "static __inline int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)\n"
            "{ SleepConditionVariableCS(c, m, INFINITE); return 0; }\n"
            "\n"
            "static __inline int pthread_cond_broadcast(pthread_cond_t *c)\n"
            "{ WakeAllConditionVariable(c); return 0; }\n"
            "\n"
            "static __inline int pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m,\n"
            "    const struct timespec *abstime)\n"
            "{\n"
            "    struct timespec now;\n"
            "    clock_gettime(CLOCK_REALTIME, &now);\n"
            "    __int64 delta_ms = (__int64)(abstime->tv_sec - now.tv_sec) * 1000\n"
            "                    + (abstime->tv_nsec - now.tv_nsec) / 1000000;\n"
            "    if (delta_ms < 0) delta_ms = 0;\n"
            "    if (delta_ms >= INFINITE) delta_ms = INFINITE - 1;\n"
            "    if (!SleepConditionVariableCS(c, m, (DWORD)delta_ms))\n"
            "        return ETIMEDOUT;\n"
            "    return 0;\n"
            "}\n"
            "\n"
            "/* ------- thread ------- */\n"
            "\n"
            "static __inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr,\n"
            "    void *(*start_routine)(void *), void *arg)\n"
            "{\n"
            "    (void)attr;\n"
            "    DWORD tid;\n"
            "    *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, &tid);\n"
            "    return (*thread != NULL) ? 0 : EAGAIN;\n"
            "}\n"
            "\n"
            "static __inline int pthread_join(pthread_t thread, void **retval)\n"
            "{\n"
            "    WaitForSingleObject(thread, INFINITE);\n"
            "    if (retval != NULL) {\n"
            "        DWORD code;\n"
            "        if (GetExitCodeThread(thread, &code))\n"
            "            *retval = (void *)(uintptr_t)code;\n"
            "        else\n"
            "            *retval = NULL;\n"
            "    }\n"
            "    CloseHandle(thread);\n"
            "    return 0;\n"
            "}\n"
            "\n"
            "#ifdef __cplusplus\n"
            "}\n"
            "#endif\n"
            "\n"
            "#endif // _PTHREAD_H_WIN32_\n"
        )

        foreach(_nexus_uvc_tgt uvc_static uvc)
            if(TARGET ${_nexus_uvc_tgt})
                get_target_property(_nexus_uvc_tgt_type ${_nexus_uvc_tgt} TYPE)
                if(NOT _nexus_uvc_tgt_type STREQUAL "ALIAS_LIBRARY")
                    target_include_directories(${_nexus_uvc_tgt} BEFORE PRIVATE "${_nexus_uvc_compat_dir}")
                endif()
            endif()
        endforeach()
    endif()

    # Promote LibUSB to PUBLIC so consumers (nexus_camera) inherit the dependency
    if(TARGET uvc_static AND TARGET LibUSB::LibUSB)
        get_target_property(_nexus_uvc_link_libs uvc_static INTERFACE_LINK_LIBRARIES)
        if(NOT _nexus_uvc_link_libs MATCHES "LibUSB")
            target_link_libraries(uvc_static PUBLIC LibUSB::LibUSB)
        endif()
    endif()
    if(TARGET uvc AND TARGET LibUSB::LibUSB)
        get_target_property(_nexus_uvc_shared_link_libs uvc INTERFACE_LINK_LIBRARIES)
        if(NOT _nexus_uvc_shared_link_libs MATCHES "LibUSB")
            target_link_libraries(uvc PUBLIC LibUSB::LibUSB)
        endif()
    endif()

    if(_nexus_libuvc_had_skip_install_rules_cache)
        set(CMAKE_SKIP_INSTALL_RULES "${_nexus_libuvc_skip_install_rules_cache}" CACHE BOOL "" FORCE)
    else()
        unset(CMAKE_SKIP_INSTALL_RULES CACHE)
    endif()
    if(_nexus_libuvc_had_warn_deprecated_cache)
        set(CMAKE_WARN_DEPRECATED "${_nexus_libuvc_warn_deprecated_cache}" CACHE BOOL "" FORCE)
    else()
        unset(CMAKE_WARN_DEPRECATED CACHE)
    endif()
    unset(_nexus_libuvc_had_warn_deprecated_cache)
    unset(_nexus_libuvc_warn_deprecated_cache)
    unset(_nexus_libuvc_had_skip_install_rules_cache)
    unset(_nexus_libuvc_skip_install_rules_cache)

    if(NOT TARGET uvc AND TARGET uvc_static)
        add_library(uvc ALIAS uvc_static)
    endif()

    # Install uvc_static into NexusKit export so CMake install(EXPORT) resolves
    if(TARGET uvc_static)
        get_target_property(_nexus_uvc_type uvc_static TYPE)
        if(NOT _nexus_uvc_type STREQUAL "ALIAS_LIBRARY")
            install(TARGETS uvc_static
                EXPORT NexusKitTargets
                ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
                PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            )
        endif()
    endif()
elseif(NOT TARGET uvc)
    find_package(uvc QUIET)
    if(NOT TARGET uvc)
        message(STATUS "libuvc not found — camera module will build without UVC backend")
    endif()
endif()
