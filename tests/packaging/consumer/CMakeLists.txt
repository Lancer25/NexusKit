cmake_minimum_required(VERSION 3.24)

project(NexusKitConsumerSmoke LANGUAGES CXX)

find_package(NexusKit CONFIG REQUIRED)

add_executable(nexuskit_consumer_smoke main.cpp)
target_compile_features(nexuskit_consumer_smoke PRIVATE cxx_std_17)
target_link_libraries(nexuskit_consumer_smoke
    PRIVATE
        nexus::nexus_core
        nexus::nexus_log
        nexus::nexus_common
        nexus::nexus_net
)
