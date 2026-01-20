cmake_minimum_required(VERSION 3.10)
project(Game)

set(CMAKE_CXX_STANDARD 17)

# Game source files
add_executable(game
    
    game/main.cpp
    "game/Actions.h"  
)

# Boss game files
add_executable(boss
    boss/bossGame.cpp
    boss/bossActions.h 
    boss/TagComponent.h
    boss/ProjectileComponent.h
    boss/PlayerShootComponent.h
    boss/DashComponent.h
 "boss/HealthComponent.h" "boss/BossComponent.h" "boss/SinusoidalProjectileComponent.h")

# Server executable
add_executable(server
    server/server.cpp
)  

# Include the engine headers (you already set up include/engine/ in the engine)
target_include_directories(game PRIVATE
    ${CMAKE_SOURCE_DIR}/CSC-481-Engine-Design/include
)

target_include_directories(server PRIVATE
    ${CMAKE_SOURCE_DIR}/CSC-481-Engine-Design/include
)

target_include_directories(boss PRIVATE 
    ${CMAKE_SOURCE_DIR}/CSC-481-Engine-Design/include
)

# Link the game against engine library
target_link_libraries(game PRIVATE engine_lib)

target_link_libraries(server PRIVATE engine_lib)

target_link_libraries(boss PRIVATE engine_lib)

# Copy asset files to the build directory
add_custom_command(
    TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:game>/assets
)
