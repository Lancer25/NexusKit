include_guard(GLOBAL)

include(CMakeParseArguments)
include(ExternalProject)

set(NEXUS_FFMPEG_INSTALL_DIR "${CMAKE_BINARY_DIR}/deps/ffmpeg" CACHE PATH "FFmpeg install prefix")
set(NEXUS_FFMPEG_ENABLE_GPL OFF CACHE BOOL "Enable GPL components in the FFmpeg source build")
set(NEXUS_FFMPEG_EXTRA_CONFIGURE_OPTIONS "" CACHE STRING "Extra options passed to FFmpeg configure")

set(NEXUS_FFMPEG_COMPONENTS
    avutil
    swresample
    swscale
    avcodec
    avformat
    avfilter
    avdevice
)

function(nexus_define_ffmpeg_targets)
    set(options)
    set(oneValueArgs SOURCE_TARGET)
    set(multiValueArgs)
    cmake_parse_arguments(NEXUS_DEFINE_FFMPEG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(NEXUS_FFMPEG_INCLUDE_DIR "${NEXUS_FFMPEG_INSTALL_DIR}/include")
    file(MAKE_DIRECTORY "${NEXUS_FFMPEG_INCLUDE_DIR}")

    if(WIN32)
        set(_NEXUS_FFMPEG_RUNTIME_DIR "${NEXUS_FFMPEG_INSTALL_DIR}/bin")
        set(_NEXUS_FFMPEG_IMPLIB_DIR "${NEXUS_FFMPEG_INSTALL_DIR}/bin")
        set(_NEXUS_FFMPEG_DLL_SUFFIXES
            avutil-59
            swresample-5
            swscale-8
            avcodec-61
            avformat-61
            avfilter-10
            avdevice-61
        )

        set(_NEXUS_FFMPEG_RUNTIME_FILES)
        foreach(_NEXUS_FFMPEG_DLL_NAME IN LISTS _NEXUS_FFMPEG_DLL_SUFFIXES)
            list(APPEND _NEXUS_FFMPEG_RUNTIME_FILES "${_NEXUS_FFMPEG_RUNTIME_DIR}/${_NEXUS_FFMPEG_DLL_NAME}.dll")
        endforeach()

        set(_NEXUS_MSYS2_UCRT_RUNTIME_DIR "C:/msys64/ucrt64/bin" CACHE PATH "MSYS2 UCRT64 runtime directory used by FFmpeg")
        foreach(_NEXUS_FFMPEG_RUNTIME_DEP libiconv-2.dll libwinpthread-1.dll zlib1.dll)
            if(EXISTS "${_NEXUS_MSYS2_UCRT_RUNTIME_DIR}/${_NEXUS_FFMPEG_RUNTIME_DEP}")
                list(APPEND _NEXUS_FFMPEG_RUNTIME_FILES "${_NEXUS_MSYS2_UCRT_RUNTIME_DIR}/${_NEXUS_FFMPEG_RUNTIME_DEP}")
            endif()
        endforeach()
    else()
        set(_NEXUS_FFMPEG_RUNTIME_DIR "${NEXUS_FFMPEG_INSTALL_DIR}/lib")
        set(_NEXUS_FFMPEG_RUNTIME_FILES)
    endif()

    set(NEXUS_FFMPEG_RUNTIME_FILES ${_NEXUS_FFMPEG_RUNTIME_FILES} PARENT_SCOPE)

    list(LENGTH NEXUS_FFMPEG_COMPONENTS _NEXUS_FFMPEG_COMPONENT_COUNT)
    math(EXPR _NEXUS_FFMPEG_LAST_INDEX "${_NEXUS_FFMPEG_COMPONENT_COUNT} - 1")

    foreach(_NEXUS_FFMPEG_INDEX RANGE 0 ${_NEXUS_FFMPEG_LAST_INDEX})
        list(GET NEXUS_FFMPEG_COMPONENTS ${_NEXUS_FFMPEG_INDEX} _NEXUS_FFMPEG_COMPONENT)

        if(TARGET FFmpeg::${_NEXUS_FFMPEG_COMPONENT})
            continue()
        endif()

        if(WIN32)
            add_library(FFmpeg::${_NEXUS_FFMPEG_COMPONENT} SHARED IMPORTED GLOBAL)
        else()
            add_library(FFmpeg::${_NEXUS_FFMPEG_COMPONENT} UNKNOWN IMPORTED GLOBAL)
        endif()

        if(NEXUS_DEFINE_FFMPEG_SOURCE_TARGET)
            add_dependencies(FFmpeg::${_NEXUS_FFMPEG_COMPONENT} ${NEXUS_DEFINE_FFMPEG_SOURCE_TARGET})
        endif()

        set_target_properties(FFmpeg::${_NEXUS_FFMPEG_COMPONENT}
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${NEXUS_FFMPEG_INCLUDE_DIR}"
        )

        if(WIN32)
            list(GET _NEXUS_FFMPEG_DLL_SUFFIXES ${_NEXUS_FFMPEG_INDEX} _NEXUS_FFMPEG_DLL_NAME)
            set_target_properties(FFmpeg::${_NEXUS_FFMPEG_COMPONENT}
                PROPERTIES
                    IMPORTED_IMPLIB "${_NEXUS_FFMPEG_IMPLIB_DIR}/${_NEXUS_FFMPEG_COMPONENT}.lib"
                    IMPORTED_LOCATION "${_NEXUS_FFMPEG_RUNTIME_DIR}/${_NEXUS_FFMPEG_DLL_NAME}.dll"
            )
        else()
            set_target_properties(FFmpeg::${_NEXUS_FFMPEG_COMPONENT}
                PROPERTIES
                    IMPORTED_LOCATION "${NEXUS_FFMPEG_INSTALL_DIR}/lib/lib${_NEXUS_FFMPEG_COMPONENT}.so"
            )
        endif()
    endforeach()

    if(NOT TARGET FFmpeg::FFmpeg)
        add_library(FFmpeg::FFmpeg INTERFACE IMPORTED GLOBAL)
        target_link_libraries(FFmpeg::FFmpeg
            INTERFACE
                FFmpeg::avutil
                FFmpeg::swresample
                FFmpeg::swscale
                FFmpeg::avcodec
                FFmpeg::avformat
                FFmpeg::avfilter
                FFmpeg::avdevice
        )
    endif()
endfunction()

if(NOT NEXUS_BUILD_DEPS OR NOT NEXUS_BUILD_FFMPEG)
    if(EXISTS "${NEXUS_FFMPEG_INSTALL_DIR}/include/libavutil/avutil.h")
        nexus_print_dependency_mode(FFmpeg "user-provided ${NEXUS_FFMPEG_INSTALL_DIR}")
        nexus_define_ffmpeg_targets()
    else()
        nexus_print_dependency_mode(FFmpeg "user-provided")
    endif()
    return()
endif()

nexus_print_dependency_mode(FFmpeg "ExternalProject n7.0.1")

set(NEXUS_FFMPEG_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/ffmpeg/src/nexus_ffmpeg_source")
file(TO_CMAKE_PATH "${NEXUS_FFMPEG_INSTALL_DIR}" NEXUS_FFMPEG_INSTALL_DIR_CMAKE)
file(TO_CMAKE_PATH "${NEXUS_FFMPEG_SOURCE_DIR}" NEXUS_FFMPEG_SOURCE_DIR_CMAKE)

if(WIN32)
    set(NEXUS_MSYS2_ENV_PATH "/ucrt64/bin:/usr/bin:/bin" CACHE STRING "PATH used inside MSYS2 bash for FFmpeg source builds")

    find_program(
        NEXUS_MSYS2_BASH_EXECUTABLE
        bash
        PATHS "C:/msys64/usr/bin" "C:/msys64/clang64/bin" "C:/msys64/ucrt64/bin"
        NO_DEFAULT_PATH
    )
    if(NOT NEXUS_MSYS2_BASH_EXECUTABLE)
        message(FATAL_ERROR "FFmpeg source builds on Windows require MSYS2 bash. Install MSYS2 or configure with -DNEXUS_BUILD_FFMPEG=OFF.")
    endif()

    find_program(
        NEXUS_NASM_EXECUTABLE
        nasm
        PATHS "C:/msys64/usr/bin" "C:/msys64/clang64/bin" "C:/msys64/ucrt64/bin"
        NO_DEFAULT_PATH
    )
    if(NOT NEXUS_NASM_EXECUTABLE)
        message(FATAL_ERROR "FFmpeg source builds require NASM. Install NASM in MSYS2 or configure with -DNEXUS_BUILD_FFMPEG=OFF.")
    endif()

    set(NEXUS_MSYS2_COMMAND_ENV "MSYSTEM=UCRT64 MINGW_PREFIX=/ucrt64 PATH='${NEXUS_MSYS2_ENV_PATH}'")

    execute_process(
        COMMAND "${NEXUS_MSYS2_BASH_EXECUTABLE}" -lc "${NEXUS_MSYS2_COMMAND_ENV} command -v make >/dev/null && ${NEXUS_MSYS2_COMMAND_ENV} command -v nproc >/dev/null && ${NEXUS_MSYS2_COMMAND_ENV} command -v nasm >/dev/null && (${NEXUS_MSYS2_COMMAND_ENV} command -v gcc >/dev/null || ${NEXUS_MSYS2_COMMAND_ENV} command -v clang >/dev/null || ${NEXUS_MSYS2_COMMAND_ENV} command -v cl >/dev/null)"
        RESULT_VARIABLE NEXUS_FFMPEG_WINDOWS_PREREQ_RESULT
    )
    if(NOT NEXUS_FFMPEG_WINDOWS_PREREQ_RESULT EQUAL 0)
        message(FATAL_ERROR "FFmpeg source builds on Windows require make, nproc, nasm, and a C compiler inside the selected MSYS2 bash environment.")
    endif()

    set(NEXUS_FFMPEG_CONFIGURE_WRAPPER "${NEXUS_MSYS2_BASH_EXECUTABLE}" -lc)
    set(NEXUS_FFMPEG_CONFIGURE_SCRIPT_PREFIX "")
    set(NEXUS_FFMPEG_COMMAND_ENV "${NEXUS_MSYS2_COMMAND_ENV}")
    set(NEXUS_FFMPEG_MAKE_COMMAND "${NEXUS_MSYS2_BASH_EXECUTABLE}" -lc "cd '${NEXUS_FFMPEG_SOURCE_DIR_CMAKE}' && ${NEXUS_FFMPEG_COMMAND_ENV} make -j$(nproc)")
    set(NEXUS_FFMPEG_INSTALL_COMMAND "${NEXUS_MSYS2_BASH_EXECUTABLE}" -lc "cd '${NEXUS_FFMPEG_SOURCE_DIR_CMAKE}' && ${NEXUS_FFMPEG_COMMAND_ENV} make install")
else()
    find_program(NEXUS_BASH_EXECUTABLE bash)
    find_program(NEXUS_MAKE_EXECUTABLE make)
    find_program(NEXUS_PKG_CONFIG_EXECUTABLE pkg-config)
    find_program(NEXUS_NASM_EXECUTABLE nasm)

    if(NOT NEXUS_BASH_EXECUTABLE OR NOT NEXUS_MAKE_EXECUTABLE OR NOT NEXUS_PKG_CONFIG_EXECUTABLE OR NOT NEXUS_NASM_EXECUTABLE)
        message(FATAL_ERROR "FFmpeg source builds require bash, make, pkg-config, and nasm.")
    endif()

    set(NEXUS_FFMPEG_CONFIGURE_WRAPPER "${NEXUS_BASH_EXECUTABLE}" -lc)
    set(NEXUS_FFMPEG_CONFIGURE_SCRIPT_PREFIX "")
    set(NEXUS_FFMPEG_COMMAND_ENV "")
    set(NEXUS_FFMPEG_MAKE_COMMAND "${NEXUS_MAKE_EXECUTABLE}" -j)
    set(NEXUS_FFMPEG_INSTALL_COMMAND "${NEXUS_MAKE_EXECUTABLE}" install)
endif()

set(NEXUS_FFMPEG_GPL_OPTION "")
if(NEXUS_FFMPEG_ENABLE_GPL)
    set(NEXUS_FFMPEG_GPL_OPTION "--enable-gpl")
endif()

set(NEXUS_FFMPEG_CONFIGURE_SCRIPT
    "${NEXUS_FFMPEG_CONFIGURE_SCRIPT_PREFIX}cd '${NEXUS_FFMPEG_SOURCE_DIR_CMAKE}' && ${NEXUS_FFMPEG_COMMAND_ENV} ./configure --prefix='${NEXUS_FFMPEG_INSTALL_DIR_CMAKE}' --disable-doc --disable-programs --disable-debug --enable-shared ${NEXUS_FFMPEG_GPL_OPTION} ${NEXUS_FFMPEG_EXTRA_CONFIGURE_OPTIONS}"
)

ExternalProject_Add(
    nexus_ffmpeg_source
    GIT_REPOSITORY https://github.com/FFmpeg/FFmpeg.git
    GIT_TAG 47f70eda3e2ff003a787e512afd07b0c266f7a70
    GIT_CONFIG ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
    PREFIX "${CMAKE_BINARY_DIR}/_deps/ffmpeg"
    SOURCE_DIR "${NEXUS_FFMPEG_SOURCE_DIR}"
    CONFIGURE_COMMAND ${NEXUS_FFMPEG_CONFIGURE_WRAPPER} "${NEXUS_FFMPEG_CONFIGURE_SCRIPT}"
    BUILD_COMMAND ${NEXUS_FFMPEG_MAKE_COMMAND}
    INSTALL_COMMAND ${NEXUS_FFMPEG_INSTALL_COMMAND}
)

nexus_define_ffmpeg_targets(SOURCE_TARGET nexus_ffmpeg_source)
