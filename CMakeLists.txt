cmake_minimum_required(VERSION 3.25)

project(
    midi-game
)

set(ROOT_DIR ${PROJECT_SOURCE_DIR})

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 20)


add_executable(${PROJECT_NAME}
    code/main.cpp
    code/miniaudio.cpp
)

# set_property(GLOBAL PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(rtmidi)
target_link_libraries(${PROJECT_NAME} PUBLIC rtmidi)

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:rtmidi>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM
)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
