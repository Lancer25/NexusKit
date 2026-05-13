include_guard(GLOBAL)

include(ExternalProject)

if(NOT NEXUS_BUILD_DEPS OR NOT NEXUS_BUILD_FFMPEG)
    nexus_print_dependency_mode(FFmpeg "user-provided")
    return()
endif()

nexus_print_dependency_mode(FFmpeg "ExternalProject n7.0.1")

set(NEXUS_FFMPEG_INSTALL_DIR "${CMAKE_BINARY_DIR}/deps/ffmpeg" CACHE PATH "FFmpeg source build install prefix")
set(NEXUS_FFMPEG_ENABLE_GPL OFF CACHE BOOL "Enable GPL components in the FFmpeg source build")
set(NEXUS_FFMPEG_EXTRA_CONFIGURE_OPTIONS "" CACHE STRING "Extra options passed to FFmpeg configure")
set(NEXUS_FFMPEG_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/ffmpeg/src/nexus_ffmpeg_source")
file(TO_CMAKE_PATH "${NEXUS_FFMPEG_INSTALL_DIR}" NEXUS_FFMPEG_INSTALL_DIR_CMAKE)
file(TO_CMAKE_PATH "${NEXUS_FFMPEG_SOURCE_DIR}" NEXUS_FFMPEG_SOURCE_DIR_CMAKE)

if(WIN32)
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

    execute_process(
        COMMAND "${NEXUS_MSYS2_BASH_EXECUTABLE}" -lc "command -v make >/dev/null && command -v nproc >/dev/null && command -v nasm >/dev/null && (command -v gcc >/dev/null || command -v clang >/dev/null || command -v cl >/dev/null)"
        RESULT_VARIABLE NEXUS_FFMPEG_WINDOWS_PREREQ_RESULT
    )
    if(NOT NEXUS_FFMPEG_WINDOWS_PREREQ_RESULT EQUAL 0)
        message(FATAL_ERROR "FFmpeg source builds on Windows require make, nproc, nasm, and a C compiler inside the selected MSYS2 bash environment.")
    endif()

    set(NEXUS_FFMPEG_CONFIGURE_WRAPPER "${NEXUS_MSYS2_BASH_EXECUTABLE}" -lc)
    set(NEXUS_FFMPEG_MAKE_COMMAND "${NEXUS_MSYS2_BASH_EXECUTABLE}" -lc "make -j$(nproc)")
    set(NEXUS_FFMPEG_INSTALL_COMMAND "${NEXUS_MSYS2_BASH_EXECUTABLE}" -lc "make install")
else()
    find_program(NEXUS_BASH_EXECUTABLE bash)
    find_program(NEXUS_MAKE_EXECUTABLE make)
    find_program(NEXUS_PKG_CONFIG_EXECUTABLE pkg-config)
    find_program(NEXUS_NASM_EXECUTABLE nasm)

    if(NOT NEXUS_BASH_EXECUTABLE OR NOT NEXUS_MAKE_EXECUTABLE OR NOT NEXUS_PKG_CONFIG_EXECUTABLE OR NOT NEXUS_NASM_EXECUTABLE)
        message(FATAL_ERROR "FFmpeg source builds require bash, make, pkg-config, and nasm.")
    endif()

    set(NEXUS_FFMPEG_CONFIGURE_WRAPPER "${NEXUS_BASH_EXECUTABLE}" -lc)
    set(NEXUS_FFMPEG_MAKE_COMMAND "${NEXUS_MAKE_EXECUTABLE}" -j)
    set(NEXUS_FFMPEG_INSTALL_COMMAND "${NEXUS_MAKE_EXECUTABLE}" install)
endif()

set(NEXUS_FFMPEG_GPL_OPTION "")
if(NEXUS_FFMPEG_ENABLE_GPL)
    set(NEXUS_FFMPEG_GPL_OPTION "--enable-gpl")
endif()

set(NEXUS_FFMPEG_CONFIGURE_SCRIPT
    "cd '${NEXUS_FFMPEG_SOURCE_DIR_CMAKE}' && ./configure --prefix='${NEXUS_FFMPEG_INSTALL_DIR_CMAKE}' --disable-doc --disable-programs --disable-debug --enable-shared ${NEXUS_FFMPEG_GPL_OPTION} ${NEXUS_FFMPEG_EXTRA_CONFIGURE_OPTIONS}"
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
