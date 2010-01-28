if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   add_definitions (-fsigned-char)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

include_directories(${CMAKE_SOURCE_DIR}/src/thirdparty/v8/include)
link_directories(${CMAKE_SOURCE_DIR}/src/thirdparty/v8)

#add_definitions(-DUSE_JPEG2000)

add_definitions (-DCLIENT)
add_executable(Intensity_CClient ../engine/3dgui ../engine/blob ../engine/blend ../engine/menus ../engine/serverbrowser ../intensity/editing_system ../intensity/messages ../intensity/logging ../intensity/message_system ../intensity/system_manager ../intensity/python_wrap ../intensity/utility ../intensity/client_system ../intensity/client_engine_additions ../intensity/character_render ../fpsgame/fps ../fpsgame/server ../fpsgame/client ../fpsgame/entities ../fpsgame/render ../fpsgame/weapon ../shared/tools ../shared/geom ../engine/rendertext ../engine/material ../engine/octaedit ../engine/grass ../engine/physics ../engine/rendergl ../engine/worldio ../engine/texture ../engine/console ../engine/world ../engine/glare ../engine/renderva ../engine/normal ../engine/rendermodel ../engine/shadowmap ../engine/main ../engine/bih ../engine/octa ../engine/lightmap ../engine/water ../engine/shader ../engine/rendersky ../engine/cubeloader ../engine/renderparticles ../engine/octarender ../engine/server ../engine/client ../engine/dynlight ../engine/decal ../engine/sound ../engine/pvs ../engine/command ../intensity/engine_additions ../intensity/world_system ../intensity/targeting ../intensity/steering ../intensity/network_system ../intensity/script_engine_manager ../intensity/script_engine ../intensity/script_engine_v8 ../intensity/fpsclient_interface ../intensity/fpsserver_interface ../intensity/master ../intensity/intensity_gui ../shared/stream ../shared/zip ../engine/movie ../intensity/shared_module_members_boost ../fpsgame/scoreboard ${Extra_ClientServer_Sources})
target_link_libraries(Intensity_CClient enet v8 ${PYTHON_LIBRARIES} ${Boost_Python_Library} ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLSOUND_LIBRARY} ${OPENGL_LIBRARIES} ${ZLIB_LIBRARIES})

if(${WIN32})
    target_link_libraries(opengl32 glu32 ws2_32 winmm msvcrt user32 imagehlp) # Intensity_CClient?
endif(${WIN32})

if(${CMAKE_SYSTEM_NAME} MATCHES "BSD")
    target_link_libraries(Intensity_CClient execinfo)
endif(${CMAKE_SYSTEM_NAME} MATCHES "BSD")

