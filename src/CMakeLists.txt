add_executable(SDLTestApp
   Main.cpp
   vk_engine.cpp
   vk_engine.h
   vk_mesh.cpp
   vk_mesh.h
   vk_types.h
   vk_initializers.cpp
   vk_initializers.h)
target_include_directories(SDLTestApp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(SDLTestApp Vulkan::Vulkan sdl2)
target_link_libraries(SDLTestApp vkbootstrap vma)

add_dependencies(SDLTestApp Shaders)
