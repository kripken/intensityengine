add_definitions (-DINTENSITY_PLUGIN)

set(PLUGIN_SOURCES "")
foreach(source ${CLIENT_SOURCES})
    set(PLUGIN_SOURCES ${PLUGIN_SOURCES} ../${source})
endforeach(source)

# If LINUX etc. ... TODO
    find_package(PkgConfig)
    pkg_check_modules(GTK gtk+-2.0)
    include_directories(${GTK_INCLUDE_DIRS})

    add_definitions(-DXP_UNIX)
    add_definitions(-DMOZ_X11)
# endif

include_directories(. ./third_party/npapi/include)

add_definitions(-DO3D_PLUGIN_NAME="IntensityEngine")
add_definitions(-DO3D_PLUGIN_DESCRIPTION="The open source Intensity Engine plugin lets you play 3D games in your browser.")
add_definitions(-DO3D_PLUGIN_MIME_TYPE="application/x-intensityengine:ie:IntensityEngine media")

add_library(IntensityPlugin SHARED
    ${PLUGIN_SOURCES}
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
    plugin/cross/main.cc
    plugin/linux/main_linux.cc
    third_party/nixysa/static_glue/npapi/npn_api.cc
    third_party/nixysa/static_glue/npapi/common.cc
)
target_link_libraries(IntensityPlugin ${CLIENT_LIBRARIES})

