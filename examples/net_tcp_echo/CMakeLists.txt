add_executable(nexus_example_net_tcp_echo
    main.cpp
)

target_link_libraries(nexus_example_net_tcp_echo
    PRIVATE
        nexus::net
)
