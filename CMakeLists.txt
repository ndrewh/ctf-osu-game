cmake_minimum_required(VERSION 3.12)
set(CMAKE_BUILD_TYPE Debug)

project(osu)
# set(CMAKE_CXX_FLAGS_DEBUG "-O1")
add_executable(osu
    src/circle_element.cpp
    src/main.cpp
    src/menu.cpp
    src/shaders.cpp
    src/level.cpp
    src/overlay.cpp
    src/glad/glad/glad.c
    src/audio.cpp
    src/TinySoundFont/examples/minisdl_audio.c
    src/level_save.cpp
    src/replay.cpp
)
IF(APPLE)
	# TARGET_LINK_LIBRARIES(MantaMIDI "-framework CoreMidi")
    TARGET_LINK_LIBRARIES(osu "-framework CoreAudio")
    TARGET_LINK_LIBRARIES(osu "-framework CoreServices")
    TARGET_LINK_LIBRARIES(osu "-framework AudioToolbox")
    TARGET_LINK_LIBRARIES(osu "-framework CoreFoundation")
	TARGET_LINK_LIBRARIES(osu ${SYSTEM_LIBS})
ENDIF(APPLE)
set_property(TARGET osu PROPERTY CXX_STANDARD 20)

add_subdirectory(glfw)
include_directories(glfw/include)

find_package(OpenGL REQUIRED)
target_link_libraries(osu glfw ${GLFW_LIBRARIES})
set_target_properties(osu PROPERTIES COMPILE_FAGS -DBUILDER_STATIC_DEFINE)
# target_link_libraries(osu OpenGL::GL)
target_include_directories(osu PRIVATE src/glad/)
