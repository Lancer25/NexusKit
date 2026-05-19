include(CppHttplib)
include(Asio)
include(Websocketpp)

add_library(nexus_net ${NEXUS_LIBRARY_TYPE}
    http.cpp
    tcp.cpp
    tcp_listener.cpp
    udp.cpp
    websocket_client.cpp
    websocket_server.cpp
)

add_library(nexus::net ALIAS nexus_net)

target_include_directories(nexus_net
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(nexus_net PUBLIC cxx_std_17)

target_link_libraries(nexus_net
    PUBLIC
        nexus::core
    PRIVATE
        nexus::common
        nexus::log
        $<BUILD_INTERFACE:asio::asio>
        $<BUILD_INTERFACE:httplib::httplib>
        $<BUILD_INTERFACE:websocketpp::websocketpp>
)

# --- TLS support (opt-in, requires OpenSSL) ---
if(OpenSSL_FOUND OR NEXUS_BUILD_OPENSSL)
    target_compile_definitions(nexus_net PRIVATE NEXUS_NET_HAS_TLS)
    if(TARGET OpenSSL::SSL)
        target_link_libraries(nexus_net PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
endif()

if(NEXUS_BUILD_SHARED)
    target_compile_definitions(nexus_net
        PUBLIC NEXUS_NET_SHARED
        PRIVATE NEXUS_NET_BUILDING_LIBRARY
    )
endif()

if(MSVC)
    target_compile_options(nexus_net PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_net PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(nexus_net PROPERTIES
    OUTPUT_NAME nexus_net
    EXPORT_NAME net
)

install(
    TARGETS nexus_net
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
