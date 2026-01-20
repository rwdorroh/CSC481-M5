cmake_minimum_required(VERSION 3.10)

# Not sure if this works for all of us, but it did for me on the boiler plate
# set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

project(Engine)

set(CMAKE_CXX_STANDARD 17)

# The Engine library
add_library(engine_lib STATIC
    src/Engine.cpp
    
    src/Input.cpp
    src/Client.cpp
    src/Timeline.cpp
    src/RenderComponent.cpp
    src/EventManager.cpp
    src/GameObjectPool.cpp
    src/GameObjectAllocator.cpp
)

# This is the corrected include directory. It points to the parent "include" folder
# so that the compiler can resolve includes like <engine/Engine.h>
target_include_directories(engine_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Find zeromq and cppzmq via vcpkg
find_package(cppzmq CONFIG REQUIRED)
# Link cppzmq::cppzmq already pulls in zeromq
target_link_libraries(engine_lib PUBLIC cppzmq)
# target_link_libraries(server PRIVATE cppzmq)

# Find ttf and link it
find_package(SDL3_ttf REQUIRED)
target_link_libraries(engine_lib PUBLIC SDL3_ttf::SDL3_ttf)

# Find SDL3 and link to it
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
target_link_libraries(engine_lib PUBLIC SDL3::SDL3 SDL3_image::SDL3_image)
