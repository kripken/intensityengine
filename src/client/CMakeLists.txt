if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   add_definitions (-fsigned-char)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

include_directories(${CMAKE_SOURCE_DIR}/src/thirdparty/v8/include)
link_directories(${CMAKE_SOURCE_DIR}/src/thirdparty/v8)

#add_definitions(-DUSE_JPEG2000)

add_definitions (-DCLIENT)

set(CLIENT_SOURCES
    ../engine/3dgui
    ../engine/blob
    ../engine/blend
    ../engine/menus
    ../engine/serverbrowser
    ../intensity/editing_system
    ../intensity/messages
    ../intensity/logging
    ../intensity/message_system
    ../intensity/system_manager
    ../intensity/python_wrap
    ../intensity/utility
    ../intensity/client_system
    ../intensity/client_engine_additions
    ../intensity/character_render
    ../intensity/intensity_physics
    ../intensity/intensity_physics_sauer
    ../fpsgame/fps
    ../fpsgame/server
    ../fpsgame/client
    ../fpsgame/entities
    ../fpsgame/render
    ../fpsgame/weapon
    ../shared/tools
    ../shared/geom
    ../engine/rendertext
    ../engine/material
    ../engine/octaedit
    ../engine/grass
    ../engine/physics
    ../engine/rendergl
    ../engine/worldio
    ../engine/texture
    ../engine/console
    ../engine/world
    ../engine/glare
    ../engine/renderva
    ../engine/normal
    ../engine/rendermodel
    ../engine/shadowmap
    ../engine/main
    ../engine/bih
    ../engine/octa
    ../engine/lightmap
    ../engine/water
    ../engine/shader
    ../engine/rendersky
    ../engine/cubeloader
    ../engine/renderparticles
    ../engine/octarender
    ../engine/server
    ../engine/client
    ../engine/dynlight
    ../engine/decal
    ../engine/sound
    ../engine/pvs
    ../engine/command
    ../intensity/engine_additions
    ../intensity/world_system
    ../intensity/targeting
    ../intensity/steering
    ../intensity/network_system
    ../intensity/script_engine_manager
    ../intensity/script_engine
    ../intensity/script_engine_v8
    ../intensity/fpsclient_interface
    ../intensity/fpsserver_interface
    ../intensity/master
    ../intensity/intensity_gui
    ../shared/stream
    ../shared/zip
    ../engine/movie
    ../intensity/shared_module_members_boost
    ../fpsgame/scoreboard
    ${Extra_ClientServer_Sources}
    ${BULLET_SOURCES}
)

set(CLIENT_LIBRARIES
    enet
    v8
    ${PYTHON_LIBRARIES}
    ${Boost_Python_Library}
    ${SDL_LIBRARY}
    ${SDLMIXER_LIBRARY}
    ${SDLIMAGE_LIBRARY}
    ${SDLSOUND_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${BULLET_LIBRARIES}
    ${UTIL}
)

if(${WIN32})
    set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES} opengl32 glu32 ws2_32 winmm msvcrt user32 imagehlp)
endif(${WIN32})

if(${CMAKE_SYSTEM_NAME} MATCHES "BSD")
    set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES} execinfo)
endif(${CMAKE_SYSTEM_NAME} MATCHES "BSD")

# Experimental plugin - Linux only for now
if(${INTENSITY_PLUGIN})
    message(STATUS "*** Building with experimental browser plugin ***")

    add_definitions(-DINTENSITY_PLUGIN)

    add_subdirectory(plugin)
    add_subdirectory(plugin_listener)

    include_directories(${CMAKE_SOURCE_DIR}/src/client/plugin_listener)
    set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES}
        pluginlistener
    )
    link_directories(./plugin_listener)
endif(${INTENSITY_PLUGIN})

add_executable(Intensity_CClient ${CLIENT_SOURCES})
target_link_libraries(Intensity_CClient ${CLIENT_LIBRARIES})

