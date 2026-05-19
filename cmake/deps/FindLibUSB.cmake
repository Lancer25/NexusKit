# FindLibUSB shim — used when libusb is built via ExternalProject (LibUSB.cmake)
# and the LibUSB::LibUSB IMPORTED target already exists.
if(TARGET LibUSB::LibUSB)
    set(LibUSB_FOUND TRUE)
    set(LibUSB_INCLUDE_DIRS "")
    set(LibUSB_LIBRARIES "")
    return()
endif()

# Fall through to CMake's built-in FindLibUSB if the target isn't pre-created.
find_package(LibUSB QUIET)
