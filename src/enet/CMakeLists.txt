include_directories(include)

add_definitions (-DHAS_SOCKLEN_T=1)
set(CMAKE_CXX_FLAGS $CMAKE_CXX_FLAGS "-Wno-error")

add_library(enet callbacks.c host.c list.c packet.c peer.c protocol.c unix.c win32.c)

