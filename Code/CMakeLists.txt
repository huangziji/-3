cmake_minimum_required(VERSION 3.15)
project(Bvh)

include(../build/conan_toolchain.cmake)
find_package(Bullet REQUIRED)
find_package(recastnavigation REQUIRED)

set(CMAKE_PREFIX_PATH /home/edwde/.local/micromamba/envs/Bvh/)
find_package(glfw3 REQUIRED)
find_package(glad  REQUIRED)
find_package(glm   REQUIRED)

add_definitions(-DGLFW_INCLUDE_NONE)

add_executable(Bvh main.cpp base.frag base.glsl)
target_link_libraries(Bvh glfw glad dl BvhCommon
    BulletSoftBody
    Bullet3Common
    BulletDynamics
    BulletCollision
    LinearMath
)
target_link_directories(Bvh PRIVATE
    ${CMAKE_PREFIX_PATH}/lib
    ${CMAKE_BINARY_DIR}
    ${CMAKE_LIBRARY_PATH}
)
target_include_directories(Bvh PRIVATE
    ${CMAKE_INCLUDE_PATH}
)
add_library(BvhCommon common.cpp common.h)
target_include_directories(BvhCommon PRIVATE
    ${CMAKE_PREFIX_PATH}/include
)
target_compile_options(BvhCommon PRIVATE -fPIC)

add_library(BvhPlugin SHARED plugin.cpp bvh.h rig.csv)
target_include_directories(BvhPlugin PRIVATE
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_INCLUDE_PATH}
)
target_link_directories(BvhPlugin PRIVATE
    ${CMAKE_LIBRARY_PATH}
)
target_link_libraries(BvhPlugin PRIVATE
    BulletSoftBody
    Bullet3Common
    BulletDynamics
    BulletCollision
    LinearMath
    BvhCommon
)
