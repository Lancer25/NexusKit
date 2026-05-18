include_guard(GLOBAL)

if(DEFINED ENV{HTTPS_PROXY})
    set(_NEXUS_GIT_PROXY_DEFAULT "$ENV{HTTPS_PROXY}")
elseif(DEFINED ENV{HTTP_PROXY})
    set(_NEXUS_GIT_PROXY_DEFAULT "$ENV{HTTP_PROXY}")
else()
    set(_NEXUS_GIT_PROXY_DEFAULT "")
endif()

set(NEXUS_GIT_SSL_BACKEND "openssl" CACHE STRING "Git TLS backend used by FetchContent dependency clones")
set(NEXUS_GIT_PROXY "${_NEXUS_GIT_PROXY_DEFAULT}" CACHE STRING "Optional Git proxy used by FetchContent dependency clones")
set(NEXUS_GIT_CONFIG_ARGS "http.sslBackend=${NEXUS_GIT_SSL_BACKEND}" CACHE STRING "Additional Git -c style config entries passed to FetchContent")

set(NEXUS_GIT_EFFECTIVE_CONFIG_ARGS ${NEXUS_GIT_CONFIG_ARGS})
if(NEXUS_GIT_PROXY)
    list(APPEND NEXUS_GIT_EFFECTIVE_CONFIG_ARGS "http.proxy=${NEXUS_GIT_PROXY}" "https.proxy=${NEXUS_GIT_PROXY}")
endif()

function(nexus_print_dependency_mode dependency_name mode)
    message(STATUS "Nexus dependency ${dependency_name}: ${mode}")
endfunction()
