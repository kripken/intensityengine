# Main external software sources

find_package(PythonLibs REQUIRED)
if(${WIN32})
#    set (PYTHON_INCLUDE_PATH "C:\\PYTHON26\\include")
#    set (PYTHON_LIBRARIES "C:\\PYTHON26\\libs\\python26.lib")
    message(STATUS "Py path: ${PYTHON_INCLUDE_PATH}")
    message(STATUS "Py libraries: ${PYTHON_LIBRARIES}")

    add_definitions(-DBOOST_PYTHON_STATIC_LIB)
    add_definitions(-DBOOST_AUTO_LINK_NOMANGLE)
    add_definitions(-DBOOST_PYTHON_SOURCE)
    add_definitions(-DBOOST_PYTHON_NO_LIB)
    add_definitions(-DBOOST_ALL_NO_LIB)
endif(${WIN32})

#set(BOOST_ROOT ${INTENSITY_DEP_PATH}/boost)
#message(STATUS "${BOOST_ROOT}")
#set(Boost_LIBRARYDIR "${INTENSITY_DEP_PATH}/boost/lib")
#set(CMAKE_PREFIX_PATH ${INTENSITY_DEP_PATH}/boost ${CMAKE_PREFIX_PATH})
#find_package(Boost 1.35.0 COMPONENTS python REQUIRED)

find_package(OpenGL REQUIRED)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${INTENSITY_DEP_PATH}/SDL)
find_package(SDL REQUIRED)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${INTENSITY_DEP_PATH}/SDL_mixer)
find_package(SDL_mixer REQUIRED)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${INTENSITY_DEP_PATH}/SDL_image)
find_package(SDL_image REQUIRED)

find_package(ZLIB REQUIRED)

# Internal software sources

include_directories(shared engine fpsgame intensity enet/include thirdparty/boost include ${SDL_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDLIMAGE_INCLUDE_DIR} ${PYTHON_INCLUDE_PATH})
link_directories(enet)
link_directories(thirdparty/boost/libs/python)

# Subdirs

set(Boost_Python_Library boost_python)
#if(${WIN32})
#    set(Boost_Python_Library boost_python-vc90-mt-1_41) # Must match what Boost invents for its own name, see auto_link.hpp
##    link_directories(src\\thirdparty\\boost\\libs\\python)
##    link_directories(src\\thirdparty\\boost\\libs\\python\\Release)
#endif(${WIN32})
message(STATUS "Boost.Python libname (1): ${Boost_Python_Library}")

add_subdirectory(thirdparty)
add_subdirectory(enet)

if(${INTENSITY_BULLET})
    message(STATUS "*** Building with experimental Bullet physics ***")
    add_definitions(-DINTENSITY_BULLET)
    include_directories(${CMAKE_SOURCE_DIR}/src/thirdparty/bullet/src/)
    link_directories(
        ${CMAKE_SOURCE_DIR}/src/thirdparty/bullet/src/BulletCollision
        ${CMAKE_SOURCE_DIR}/src/thirdparty/bullet/src/BulletDynamics
        ${CMAKE_SOURCE_DIR}/src/thirdparty/bullet/src/LinearMath
    )
    set(BULLET_SOURCES ../intensity/intensity_physics_bullet.cpp)
    set(BULLET_LIBRARIES BulletDynamics BulletCollision LinearMath)
endif(${INTENSITY_BULLET})

add_subdirectory(server)
add_subdirectory(client)

