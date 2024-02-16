cmake_minimum_required(VERSION 3.15)
project(Bvh)

set(CMAKE_PREFIX_PATH /home/edwde/.local/micromamba/envs/Bvh/)
add_definitions(-DGLFW_INCLUDE_NONE)
find_package(glfw3 REQUIRED)
find_package(glad  REQUIRED)
find_package(glm   REQUIRED)

add_executable(Bvh main.cpp base.frag)
target_link_libraries(Bvh glfw glad dl BvhCommon)
target_link_directories(Bvh PRIVATE
    ${CMAKE_PREFIX_PATH}/lib
    ${CMAKE_BINARY_DIR}
)
target_include_directories(Bvh PRIVATE
    ${CMAKE_PREFIX_PATH}/include
)
add_library(BvhPlugin SHARED plugin.cpp)
add_library(BvhCommon SHARED common.cpp)
target_link_directories(BvhCommon PRIVATE
    ${CMAKE_PREFIX_PATH}/lib
)
target_include_directories(BvhCommon PRIVATE
    ${CMAKE_PREFIX_PATH}/include
)
