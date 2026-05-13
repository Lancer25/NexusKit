include_guard(GLOBAL)

set(NEXUS_GIT_SSL_BACKEND "openssl" CACHE STRING "Git TLS backend used by FetchContent dependency clones")
set(NEXUS_GIT_CONFIG_ARGS "http.sslBackend=${NEXUS_GIT_SSL_BACKEND}" CACHE STRING "Git -c style config entries passed to FetchContent")

function(nexus_print_dependency_mode dependency_name mode)
    message(STATUS "Nexus dependency ${dependency_name}: ${mode}")
endfunction()
