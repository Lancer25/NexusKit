include_guard(GLOBAL)

include(ExternalProject)

if(NOT NEXUS_BUILD_DEPS OR NOT NEXUS_BUILD_FFMPEG OR NOT NEXUS_FFMPEG_ENABLE_GPL)
    return()
endif()

nexus_print_dependency_mode(x264 "ExternalProject (GPL)")

set(NEXUS_X264_INSTALL_DIR "${CMAKE_BINARY_DIR}/deps/x264" CACHE PATH "x264 install prefix")
set(NEXUS_X264_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/x264/src/nexus_x264_source")
file(TO_CMAKE_PATH "${NEXUS_X264_INSTALL_DIR}" NEXUS_X264_INSTALL_DIR_CMAKE)
file(TO_CMAKE_PATH "${NEXUS_X264_SOURCE_DIR}" NEXUS_X264_SOURCE_DIR_CMAKE)

if(WIN32)
    find_program(
        _NEXUS_X264_BASH
        bash
        PATHS "C:/msys64/usr/bin" "C:/msys64/clang64/bin" "C:/msys64/ucrt64/bin"
        NO_DEFAULT_PATH
    )
    if(NOT _NEXUS_X264_BASH)
        message(FATAL_ERROR "x264 source build on Windows requires MSYS2 bash.")
    endif()

    find_program(
        _NEXUS_X264_NASM
        nasm
        PATHS "C:/msys64/usr/bin" "C:/msys64/clang64/bin" "C:/msys64/ucrt64/bin"
        NO_DEFAULT_PATH
    )
    if(NOT _NEXUS_X264_NASM)
        message(FATAL_ERROR "x264 source build requires NASM.")
    endif()

    set(_NEXUS_X264_MSYS2_ENV "MSYSTEM=UCRT64 MINGW_PREFIX=/ucrt64 PATH='/ucrt64/bin:/usr/bin:/bin'")

    set(NEXUS_X264_CONFIGURE_COMMAND
        "${_NEXUS_X264_BASH}" -lc
        "cd '${NEXUS_X264_SOURCE_DIR_CMAKE}' && ${_NEXUS_X264_MSYS2_ENV} ./configure --prefix='${NEXUS_X264_INSTALL_DIR_CMAKE}' --enable-static --disable-shared --disable-cli --disable-opencl --host=x86_64-w64-mingw32"
    )
    set(NEXUS_X264_BUILD_COMMAND
        "${_NEXUS_X264_BASH}" -lc
        "cd '${NEXUS_X264_SOURCE_DIR_CMAKE}' && ${_NEXUS_X264_MSYS2_ENV} make -j$(nproc)"
    )
    set(NEXUS_X264_INSTALL_COMMAND
        "${_NEXUS_X264_BASH}" -lc
        "cd '${NEXUS_X264_SOURCE_DIR_CMAKE}' && ${_NEXUS_X264_MSYS2_ENV} make install"
    )

    # PKG_CONFIG_PATH for FFmpeg — use Unix-style path for MSYS2
    string(REPLACE ":" ";" _nexus_x264_pc_path_list "$ENV{PKG_CONFIG_PATH}")
    set(NEXUS_X264_PKG_CONFIG_PATH "${NEXUS_X264_INSTALL_DIR}/lib/pkgconfig")
else()
    set(NEXUS_X264_CONFIGURE_COMMAND
        ${CMAKE_COMMAND} -E chdir "${NEXUS_X264_SOURCE_DIR}" ./configure --prefix="${NEXUS_X264_INSTALL_DIR}" --enable-static --enable-pic --disable-cli --disable-opencl
    )
    set(NEXUS_X264_BUILD_COMMAND   make -j$(nproc))
    set(NEXUS_X264_INSTALL_COMMAND make install)
    set(NEXUS_X264_PKG_CONFIG_PATH "${NEXUS_X264_INSTALL_DIR}/lib/pkgconfig")
endif()

ExternalProject_Add(
    nexus_x264_source
    GIT_REPOSITORY https://code.videolan.org/videolan/x264.git
    GIT_TAG        master
    GIT_CONFIG     ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
    PREFIX         "${CMAKE_BINARY_DIR}/_deps/x264"
    CONFIGURE_COMMAND ${NEXUS_X264_CONFIGURE_COMMAND}
    BUILD_COMMAND     ${NEXUS_X264_BUILD_COMMAND}
    INSTALL_COMMAND   ${NEXUS_X264_INSTALL_COMMAND}
    BUILD_BYPRODUCTS "${NEXUS_X264_INSTALL_DIR}/lib/libx264.a"
)
