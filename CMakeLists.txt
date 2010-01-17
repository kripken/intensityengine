#
# Intensity Engine build script
#
# Notes:
# ======
#   * VC++ 2008: Will only build in Release mode (build->conf. manager-> Set the active one to Release)
#   * windows_dev/include/python.h seems to confuse CMake
#

cmake_minimum_required(VERSION 2.6)
project(Intensity)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message(STATUS "...Linux...")
    add_definitions (-DLINUX)
    set(CMAKE_CXX_FLAGS "-g -O1 -Wall -Werror")
else(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(STATUS "...OS X...")
    add_definitions (-DLINUX) # For now...
    set(CMAKE_CXX_FLAGS "-g -O1 -Wall")
    # Workarounds:
#    include_directories(/usr/X11/include)
    link_directories(src/thirdparty/v8)
else(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(STATUS "...Windows...")
    add_definitions (-DWINDOWS)
    set(CMAKE_CXX_FLAGS "/DWIN32 /O2 /EHsc")
    file(TO_CMAKE_PATH "$ENV{INTENSITY_DEP_PATH}" INTENSITY_DEP_PATH) # e.g. src/windows_dev
    include_directories(${INTENSITY_DEP_PATH}/include)
    link_directories(${INTENSITY_DEP_PATH}/lib)

    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${INTENSITY_DEP_PATH})

    file(TO_CMAKE_PATH "C:\\Program Files\\Microsoft SDKs\\Windows\\v6.0A" PLATFORM_SDK_A)
    include_directories(${PLATFORM_SDK_A}/Include)

    file(TO_CMAKE_PATH "C:\\Program Files\\Microsoft Visual Studio 9.0\\VC" PLATFORM_SDK_B)
    include_directories(${PLATFORM_SDK_B}/Include)
    message(STATUS "Set up SDKs at: ${PLATFORM_SDK_A};${PLATFORM_SDK_B}")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

add_subdirectory(src)

