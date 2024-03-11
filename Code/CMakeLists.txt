cmake_minimum_required(VERSION 3.15)
project(Ragdoll)

include(../build/conan_toolchain.cmake)
find_package(Bullet REQUIRED)
find_package(recastnavigation REQUIRED)
find_package(openfbx REQUIRED)

set(CMAKE_PREFIX_PATH /home/edwde/.local/micromamba/envs/Bvh/)
add_definitions(-DGLFW_INCLUDE_NONE)
find_package(glfw3 REQUIRED)
find_package(glad  REQUIRED)
find_package(glm   REQUIRED)

add_executable(Ragdoll main.cpp base.frag base.glsl bvh.h
    text.glsl
    bake2.frag)
target_link_libraries(Ragdoll glfw glad dl pthread
    RagdollCommon
    BulletSoftBody
    Bullet3Common
    BulletDynamics
    BulletCollision
    LinearMath
)
target_link_directories(Ragdoll PRIVATE
    ${CMAKE_PREFIX_PATH}/lib
    ${CMAKE_LIBRARY_PATH}
    ${CMAKE_BINARY_DIR}
)
target_include_directories(Ragdoll PRIVATE
    ${CMAKE_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/../External/
    ${CMAKE_CURRENT_SOURCE_DIR}/../imgui-1.90.4/
)
add_library(RagdollCommon common.cpp common.h
    miniaudio.cpp)
target_include_directories(RagdollCommon PRIVATE
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/../External/
    ${CMAKE_CURRENT_SOURCE_DIR}/../imgui-1.90.4/
)
target_compile_options(RagdollCommon PRIVATE -fPIC)

add_library(RagdollPlugin SHARED plugin.cpp rig.csv
    Anim.cpp Database.cpp)
target_include_directories(RagdollPlugin PRIVATE
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_INCLUDE_PATH}
)
target_link_directories(RagdollPlugin PRIVATE
    ${CMAKE_LIBRARY_PATH}
)
target_link_libraries(RagdollPlugin PRIVATE
    BulletSoftBody
    Bullet3Common
    BulletDynamics
    BulletCollision
    LinearMath
    RagdollCommon
    OpenFBX
    miniz
)
