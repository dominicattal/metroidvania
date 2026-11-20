cmake_minimum_required(VERSION 3.5)

project(metroid)

find_package(Freetype REQUIRED)

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
target_link_libraries(metroid PRIVATE glfw Freetype::Freetype miniaudio)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/assets/)
file(COPY ${CMAKE_HOME_DIRECTORY}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets/)
