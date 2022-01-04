add_executable(SDLTestApp
   Main.cpp)
target_include_directories(SDLTestApp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(SDLTestApp Vulkan::Vulkan sdl2)
add_dependencies(SDLTestApp Shaders)
