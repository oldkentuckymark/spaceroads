cmake_minimum_required(VERSION 3.22)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(spaceroads LANGUAGES CXX)

file(GLOB_RECURSE SPACEROADS_DATA_FILES CONFIGURE_DEPENDS "data/*")

set(LICENCES
    LICENSE
)


set(SPACEROADS_HEADERS
    source/color.hpp
    source/util.hpp
    source/ffm.hpp
    source/ffr.hpp
    source/ffp.hpp
    source/mesh.hpp
    source/player.hpp
    source/level.hpp
    source/csv.hpp
    source/cell.hpp
    source/renderer.hpp
    source/game.hpp
    source/camera.hpp

)



set(SPACEROADS_SOURCES

)

set(SPACEROADS_SDL_SOURCES
    ${SPACEROADS_SOURCES}
    source/platform/sdl/source/main.cpp
)

set(SPACEROADS_GBA_SOURCES
    ${SPACEROADS_SOURCES}
    source/platform/gba/source/main.iwram.cpp
)

add_executable(spaceroads-sdl
    ${SPACEROADS_SDL_SOURCES}
    ${SPACEROADS_HEADERS}



)


if(UNIX)
    find_library(SDL3_LIBRARY SDL3)
    find_library(PTHREADS_LIBRARY NAMES pthread)
endif()

target_compile_features(spaceroads-sdl PUBLIC cxx_std_26)
set_target_properties(spaceroads-sdl PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(spaceroads-sdl PUBLIC -freflection)
target_compile_options(spaceroads-sdl PUBLIC -fconstexpr-ops-limit=2147483647)
target_link_libraries(spaceroads-sdl ${SDL3_LIBRARY} ${PTHREAD_LIBRARY})
target_include_directories(spaceroads-sdl PUBLIC source)
target_include_directories(spaceroads-sdl PUBLIC /opt/devkitpro/libgba/include)


add_custom_target(data SOURCES ${SPACEROADS_DATA_FILES})

add_custom_target(spaceroads-gba
    COMMAND make -j 4 -C ${CMAKE_CURRENT_SOURCE_DIR}/source/platform/gba -f Makefile
    SOURCES ${SPACEROADS_GBA_SOURCES}
    COMMENT "GBA Build"
    VERBATIM
)
