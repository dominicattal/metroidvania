cmake_minimum_required(VERSION 3.5)

project(metroid)

add_subdirectory(vendor/freetype)
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/miniaudio)

add_executable(metroid
    ${CMAKE_HOME_DIRECTORY}/src/main.c
    ${CMAKE_HOME_DIRECTORY}/src/game.c
    ${CMAKE_HOME_DIRECTORY}/src/ui.c
    ${CMAKE_HOME_DIRECTORY}/src/screen.c
    ${CMAKE_HOME_DIRECTORY}/src/screens/room_1.c
    ${CMAKE_HOME_DIRECTORY}/src/screens/screen_1.c
    ${CMAKE_HOME_DIRECTORY}/src/screens/screen_2.c
    ${CMAKE_HOME_DIRECTORY}/src/render.c
    ${CMAKE_HOME_DIRECTORY}/src/audio.c
    ${CMAKE_HOME_DIRECTORY}/src/util/log.c
    ${CMAKE_HOME_DIRECTORY}/src/util/arena.c
    ${CMAKE_HOME_DIRECTORY}/src/util/json.c
    ${CMAKE_HOME_DIRECTORY}/src/util/linalg.c
    ${CMAKE_HOME_DIRECTORY}/src/util/rand.c
    ${CMAKE_HOME_DIRECTORY}/vendor/glad/src/glad.c
)

target_include_directories(metroid PRIVATE vendor/glad/include vendor/stb vendor/Nuklear)
target_link_libraries(metroid PRIVATE glfw freetype miniaudio)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/assets/)
add_custom_target(copy_data
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        "${CMAKE_HOME_DIRECTORY}/assets/"
        "${CMAKE_BINARY_DIR}/assets"
    COMMENT "Copying data"
)
add_dependencies(metroid copy_data)
