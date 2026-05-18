include_guard(GLOBAL)

include(ExternalProject)

if(NOT NEXUS_BUILD_DEPS OR NOT NEXUS_BUILD_OPENSSL)
    nexus_print_dependency_mode(OpenSSL "find_package")
    find_package(OpenSSL REQUIRED)
    return()
endif()

nexus_print_dependency_mode(OpenSSL "ExternalProject 3.3.1")

find_program(NEXUS_PERL_EXECUTABLE perl)
if(NOT NEXUS_PERL_EXECUTABLE)
    message(FATAL_ERROR "OpenSSL source builds require Perl. Install Perl or configure with -DNEXUS_BUILD_OPENSSL=OFF and provide OpenSSL through find_package.")
endif()

set(NEXUS_OPENSSL_INSTALL_DIR "${CMAKE_BINARY_DIR}/deps/openssl" CACHE PATH "OpenSSL source build install prefix")

if(WIN32)
    find_program(NEXUS_NMAKE_EXECUTABLE nmake)
    if(NOT NEXUS_NMAKE_EXECUTABLE)
        message(FATAL_ERROR "OpenSSL source builds on Windows require nmake from a Visual Studio developer environment.")
    endif()

    set(NEXUS_OPENSSL_CONFIGURE_TARGET "VC-WIN64A" CACHE STRING "OpenSSL Configure target")
    set(NEXUS_OPENSSL_CONFIGURE_COMMAND
        "${NEXUS_PERL_EXECUTABLE}" Configure "${NEXUS_OPENSSL_CONFIGURE_TARGET}" no-tests "--prefix=${NEXUS_OPENSSL_INSTALL_DIR}"
    )
    set(NEXUS_OPENSSL_BUILD_COMMAND "${NEXUS_NMAKE_EXECUTABLE}")
    set(NEXUS_OPENSSL_INSTALL_COMMAND "${NEXUS_NMAKE_EXECUTABLE}" install_sw)
else()
    find_program(NEXUS_MAKE_EXECUTABLE make)
    if(NOT NEXUS_MAKE_EXECUTABLE)
        message(FATAL_ERROR "OpenSSL source builds require make.")
    endif()

    set(NEXUS_OPENSSL_CONFIGURE_TARGET "" CACHE STRING "OpenSSL Configure target; leave empty for OpenSSL auto-detection")
    if(NEXUS_OPENSSL_CONFIGURE_TARGET)
        set(NEXUS_OPENSSL_CONFIGURE_COMMAND
            "${NEXUS_PERL_EXECUTABLE}" Configure "${NEXUS_OPENSSL_CONFIGURE_TARGET}" no-tests "--prefix=${NEXUS_OPENSSL_INSTALL_DIR}"
        )
    else()
        set(NEXUS_OPENSSL_CONFIGURE_COMMAND
            "${NEXUS_PERL_EXECUTABLE}" Configure no-tests "--prefix=${NEXUS_OPENSSL_INSTALL_DIR}"
        )
    endif()
    set(NEXUS_OPENSSL_BUILD_COMMAND "${NEXUS_MAKE_EXECUTABLE}" -j)
    set(NEXUS_OPENSSL_INSTALL_COMMAND "${NEXUS_MAKE_EXECUTABLE}" install_sw)
endif()

ExternalProject_Add(
    nexus_openssl_source
    GIT_REPOSITORY https://github.com/openssl/openssl.git
    GIT_TAG 243b18a4c9e2865caf7901ec4506e899cfc34d7c
    GIT_CONFIG ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
    PREFIX "${CMAKE_BINARY_DIR}/_deps/openssl"
    CONFIGURE_COMMAND ${NEXUS_OPENSSL_CONFIGURE_COMMAND}
    BUILD_COMMAND ${NEXUS_OPENSSL_BUILD_COMMAND}
    INSTALL_COMMAND ${NEXUS_OPENSSL_INSTALL_COMMAND}
)
