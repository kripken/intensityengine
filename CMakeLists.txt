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

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")
    message(STATUS "...Linux...")
    add_definitions (-DLINUX)
    set(CMAKE_CXX_FLAGS "-g -O1 -Wall")# -Werror") # Due to warnings in BSD
    set(UTIL util)
else(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(STATUS "...OS X...")
    add_definitions (-DLINUX) # For now... should really rename it to UNIX though
    set(CMAKE_CXX_FLAGS "-g -O1 -Wall")
    # Workarounds:
#    include_directories(/usr/X11/include)
    link_directories(src/thirdparty/v8)
    set(CMAKE_OSX_ARCHITECTURES "i386")
    set(Extra_ClientServer_Sources ../osx_dev/macutils.mm ../osx_dev/SDLMain.m)
else(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(STATUS "...Windows...")
    add_definitions (-DWINDOWS)
    set(CMAKE_CXX_FLAGS "/DWIN32 /O2 /EHsc")
    set(INTENSITY_DEP_PATH ${CMAKE_SOURCE_DIR}/src/windows)
    message(STATUS "Dependencies at:" ${INTENSITY_DEP_PATH})
    include_directories(${INTENSITY_DEP_PATH}/include)
    link_directories(${INTENSITY_DEP_PATH}/lib)

    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${INTENSITY_DEP_PATH})
    message(STATUS "Prefix path:" ${CMAKE_PREFIX_PATH})
            
    file(TO_CMAKE_PATH "C:\\Program Files\\Microsoft SDKs\\Windows\\v6.0A" PLATFORM_SDK_A)
    include_directories(${PLATFORM_SDK_A}/Include)

    file(TO_CMAKE_PATH "C:\\Program Files\\Microsoft Visual Studio 9.0\\VC" PLATFORM_SDK_B)
    include_directories(${PLATFORM_SDK_B}/Include)
    message(STATUS "Set up SDKs at: ${PLATFORM_SDK_A};${PLATFORM_SDK_B}")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")

add_subdirectory(src)

