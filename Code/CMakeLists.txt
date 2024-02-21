cmake_minimum_required(VERSION 3.15)
project(Bvh)

include(../build/conan_toolchain.cmake)
find_package(Bullet REQUIRED)

set(CMAKE_PREFIX_PATH /home/edwde/.local/micromamba/envs/Bvh/)
add_definitions(-DGLFW_INCLUDE_NONE)
find_package(glfw3 REQUIRED)
find_package(glad  REQUIRED)
find_package(glm   REQUIRED)


add_executable(Bvh main.cpp base.frag
    base.glsl)
target_link_libraries(Bvh glfw glad dl BvhCommon)
target_link_directories(Bvh PRIVATE
    ${CMAKE_PREFIX_PATH}/lib
    ${CMAKE_BINARY_DIR}
)
target_include_directories(Bvh PRIVATE
    ${CMAKE_PREFIX_PATH}/include
)
add_library(BvhPlugin SHARED plugin.cpp bvh.h)
target_link_directories(BvhPlugin PRIVATE
    ${CMAKE_LIBRARY_PATH}
)
target_include_directories(BvhPlugin PRIVATE
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_INCLUDE_PATH}
)
target_link_libraries(BvhPlugin PRIVATE
    BulletSoftBody
    Bullet3Common
    BulletDynamics
    BulletCollision
    LinearMath
)
add_library(BvhCommon common.cpp common.h)
target_include_directories(BvhCommon PRIVATE
    ${CMAKE_PREFIX_PATH}/include
)
