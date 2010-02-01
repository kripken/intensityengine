# If LINUX etc. ... TODO
    find_package(PkgConfig)
    pkg_check_modules(GTK gtk+-2.0)
    include_directories(${GTK_INCLUDE_DIRS})

    add_definitions(-DXP_UNIX)
    add_definitions(-DMOZ_X11)
# endif

include_directories(${CMAKE_SOURCE_DIR}/src/client/plugin/)
include_directories(${CMAKE_SOURCE_DIR}/src/client/plugin/third_party/npapi/include)

add_library(pluginlistener
    ../plugin/base/logging.cc
    ../plugin/base/command_line.cc
    ../plugin/base/base_switches.cc
    ../plugin/base/debug_util.cc
    ../plugin/base/debug_util_posix.cc
    ../plugin/base/string_util.cc
    ../plugin/base/string_piece.cc
    ../plugin/base/string16.cc
    ../plugin/base/atomicops_internals_x86_gcc.cc
    ../plugin/base/lock_impl_posix.cc
    ../plugin/base/process_posix.cc
    ../plugin/base/process_util_linux.cc
    ../plugin/base/process_util_posix.cc
    ../plugin/base/file_descriptor_shuffle.cc
    ../plugin/base/file_util.cc
    ../plugin/base/file_util_linux.cc
    ../plugin/base/file_util_posix.cc
    ../plugin/base/file_path.cc
    ../plugin/base/ref_counted.cc
    ../plugin/base/thread_collision_warner.cc
    ../plugin/base/message_loop.cc
    ../plugin/base/message_pump_libevent.cc
    ../plugin/base/histogram.cc
    ../plugin/base/pickle.cc
    ../plugin/base/dynamic_annotations.cc
    ../plugin/base/at_exit.cc
    ../plugin/base/lazy_instance.cc
    ../plugin/base/thread_local_posix.cc
    ../plugin/base/tracked.cc
    ../plugin/base/tracked_objects.cc
    ../plugin/ipc/ipc_switches.cc
    ../plugin/ipc/ipc_channel_posix.cc
    ../plugin/ipc/ipc_message.cc
    ../plugin/ipc/ipc_logging.cc
    ../plugin/third_party/nixysa/static_glue/npapi/npn_api.cc
    ../plugin/third_party/nixysa/static_glue/npapi/common.cc


    ../plugin/base/safe_strerror_posix.cc
    ../plugin/base/utf_string_conversions.cc
    ../plugin/base/sys_string_conversions_linux.cc
    ../plugin/base/setproctitle_linux.c
    ../plugin/base/third_party/dmg_fp/dtoa.cc
    ../plugin/base/third_party/dmg_fp/g_fmt.cc
    ../plugin/base/platform_thread_posix.cc
    ../plugin/base/message_pump_glib.cc
    ../plugin/base/message_pump_default.cc
    ../plugin/base/time.cc
    ../plugin/base/time_posix.cc
    ../plugin/base/thread_local_storage_posix.cc
    ../plugin/ipc/file_descriptor_set_posix.cc
    ../plugin/base/global_descriptors_posix.cc
    ../plugin/base/waitable_event_posix.cc
    ../plugin/base/condition_variable_posix.cc
    ../plugin/base/third_party/icu/icu_utf.cc
    ../plugin/base/third_party/nspr/prtime.cc
    ../plugin/base/sys_info_posix.cc

    intensity_plugin_listener.cpp
)
target_link_libraries(pluginlistener ${CMAKE_SOURCE_DIR}/src/client/plugin/third_party/libevent/.libs/libevent.so ${GTK_LIBRARIES})

