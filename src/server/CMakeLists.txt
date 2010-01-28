if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   add_definitions (-fsigned-char)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

include_directories(${CMAKE_SOURCE_DIR}/src/thirdparty/v8/include)
link_directories(${CMAKE_SOURCE_DIR}/src/thirdparty/v8)

#add_definitions(-DUSE_JPEG2000)

add_definitions (-DSERVER)
add_executable(Intensity_CServer ../intensity/editing_system ../shared/tools ../engine/server ../engine/serverbrowser ../fpsgame/fps ../fpsgame/server ../fpsgame/client ../fpsgame/entities ../intensity/python_wrap ../intensity/system_manager ../intensity/message_system ../intensity/server_system ../intensity/logging ../intensity/messages ../intensity/utility ../engine/world ../engine/worldio ../intensity/engine_additions ../engine/command ../engine/octa ../engine/physics ../engine/rendermodel ../engine/normal ../engine/bih ../shared/geom ../engine/client ../intensity/world_system ../engine/octaedit ../intensity/steering ../intensity/targeting ../intensity/network_system ../intensity/script_engine_manager ../intensity/script_engine ../intensity/script_engine_v8 ../intensity/fpsserver_interface ../intensity/fpsclient_interface ../engine/octarender ../fpsgame/weapon ../intensity/master ../shared/stream ../engine/pvs ../engine/blend ../shared/zip ../intensity/shared_module_members_boost ../intensity/NPC ${Extra_ClientServer_Sources})
target_link_libraries(Intensity_CServer enet v8 ${PYTHON_LIBRARIES} ${Boost_Python_Library} ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLSOUND_LIBRARY} ${ZLIB_LIBRARIES})

if(${WIN32})
    target_link_libraries(ws2_32 winmm msvcrt opengl32)
endif(${WIN32})

if(${CMAKE_SYSTEM_NAME} MATCHES "BSD")
    target_link_libraries(Intensity_CServer execinfo)
endif(${CMAKE_SYSTEM_NAME} MATCHES "BSD")

