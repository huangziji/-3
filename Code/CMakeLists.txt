cmake_minimum_required(VERSION 3.15)
project(Bvh)

set(CMAKE_PREFIX_PATH /home/edwde/.local/micromamba/envs/Bvh/)
add_definitions(-DGLFW_INCLUDE_NONE)
find_package(glfw3 REQUIRED)
find_package(glad  REQUIRED)
find_package(glm   REQUIRED)

add_executable(Bvh main.cpp base.frag)
target_link_libraries(Bvh glfw glad)
target_link_directories(Bvh PRIVATE
    ${CMAKE_PREFIX_PATH}/lib
)
target_include_directories(Bvh PRIVATE
    ${CMAKE_PREFIX_PATH}/include
)
