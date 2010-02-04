add_definitions (-DINTENSITY_PLUGIN)
add_definitions (-DINTENSITY_INSTALL_ROOT="${CMAKE_SOURCE_DIR}")
message(STATUS "install root:" ${CMAKE_SOURCE_DIR})

set(PLUGIN_SOURCES "")
foreach(source ${CLIENT_SOURCES})
    set(PLUGIN_SOURCES ${PLUGIN_SOURCES} ../${source})
endforeach(source)

# If LINUX etc. ... TODO
    find_package(PkgConfig)
    pkg_check_modules(GTK gtk+-2.0)
    include_directories(${GTK_INCLUDE_DIRS})

    find_package(PkgConfig)
    pkg_check_modules(XT xt)
    message(STATUS "XT libs:" ${XT_LIBRARIES})
    include_directories(${XT_INCLUDE_DIRS})

    add_definitions(-DXP_UNIX)
    add_definitions(-DMOZ_X11)
# endif

include_directories(. ./third_party/npapi/include ${CMAKE_SOURCE_DIR}/src/client/plugin_listener)

add_definitions(-DO3D_PLUGIN_NAME="IntensityEngine")
add_definitions(-DO3D_PLUGIN_DESCRIPTION="The open source Intensity Engine plugin lets you play 3D games in your browser.")
add_definitions(-DO3D_PLUGIN_MIME_TYPE="application/x-intensityengine:int:IntensityEngine media")

add_library(IntensityPlugin SHARED
    base/logging.cc
    base/command_line.cc
    base/base_switches.cc
    base/debug_util.cc
    base/debug_util_posix.cc
    base/string_util.cc
    base/string_piece.cc
    base/string16.cc
    base/atomicops_internals_x86_gcc.cc
    base/lock_impl_posix.cc
    base/process_posix.cc
    base/process_util_linux.cc
    base/process_util_posix.cc
    base/file_descriptor_shuffle.cc
    base/file_util.cc
    base/file_util_linux.cc
    base/file_util_posix.cc
    base/file_path.cc
    base/ref_counted.cc
    base/thread_collision_warner.cc
    base/message_loop.cc
    base/message_pump_libevent.cc
    base/histogram.cc
    base/pickle.cc
    base/dynamic_annotations.cc
    base/at_exit.cc
    base/lazy_instance.cc
    base/thread_local_posix.cc
    base/tracked.cc
    base/tracked_objects.cc
    base/time.cc
    base/time_posix.cc
    plugin/cross/o3d_glue.cc
    plugin/cross/main.cc
    plugin/linux/main_linux.cc
    third_party/nixysa/static_glue/npapi/npn_api.cc
    third_party/nixysa/static_glue/npapi/common.cc
    intensity_plugin.cpp
)
target_link_libraries(IntensityPlugin ${CLIENT_LIBRARIES} ${XT_LIBRARIES})

