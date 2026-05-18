include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS AND NEXUS_BUILD_PORTAUDIO)
    nexus_print_dependency_mode(PortAudio "FetchContent v19.7.0")

    set(PA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(PA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(PA_BUILD_SHARED ${NEXUS_BUILD_SHARED} CACHE BOOL "" FORCE)

    FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG 147dd722548358763a8b649b3e4b41dfffbcfbb6
        GIT_CONFIG ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
    )

    FetchContent_MakeAvailable(portaudio)

    if(TARGET portaudio AND NOT TARGET PortAudio::PortAudio)
        add_library(PortAudio::PortAudio ALIAS portaudio)
    elseif(TARGET PortAudio AND NOT TARGET PortAudio::PortAudio)
        add_library(PortAudio::PortAudio ALIAS PortAudio)
    endif()
else()
    nexus_print_dependency_mode(PortAudio "find_package")
    find_package(PortAudio REQUIRED CONFIG)
endif()
