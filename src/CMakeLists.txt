# Main external software sources

find_package(PythonLibs REQUIRED)
if(${WIN32})
    set (PYTHON_INCLUDE_PATH "${PYTHON_INCLUDE_PATH};C:\\PYTHON25\\include")
    message(STATUS "Py path: ${PYTHON_INCLUDE_PATH}")
endif(${WIN32})

#set(BOOST_ROOT ${INTENSITY_DEP_PATH}/boost)
#message(STATUS "${BOOST_ROOT}")
#set(Boost_LIBRARYDIR "${INTENSITY_DEP_PATH}/boost/lib")
#set(CMAKE_PREFIX_PATH ${INTENSITY_DEP_PATH}/boost ${CMAKE_PREFIX_PATH})
find_package(Boost 1.35.0 COMPONENTS python REQUIRED)

find_package(OpenGL REQUIRED)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${INTENSITY_DEP_PATH}/SDL)
find_package(SDL REQUIRED)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${INTENSITY_DEP_PATH}/SDL_mixer)
find_package(SDL_mixer REQUIRED)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${INTENSITY_DEP_PATH}/SDL_image)
find_package(SDL_image REQUIRED)

find_package(ZLIB REQUIRED)

# Internal software sources

include_directories(shared engine fpsgame intensity enet/include ${Boost_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDLIMAGE_INCLUDE_DIR} ${PYTHON_INCLUDE_PATH})
link_directories(enet)

# Subdirs

add_subdirectory(enet)
add_subdirectory(server)
add_subdirectory(client)

