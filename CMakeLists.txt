cmake_minimum_required(VERSION 3.10)
project(nixum_install)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_EXTENSIONS ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_executable(nixum_install main.cpp)

target_include_directories(nixum_install PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
target_link_libraries(nixum_install PRIVATE ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} SDL2_ttf)

# Verify that the font file exists and copy it to the build directory
add_custom_command(TARGET nixum_install POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/test.ttf"
        $<TARGET_FILE_DIR:nixum_install>
    VERBATIM
    COMMAND ${CMAKE_COMMAND} -E echo "Font file copied successfully."
    COMMENT "Copying font file to build directory."
    COMMAND_EXPAND_LISTS)
