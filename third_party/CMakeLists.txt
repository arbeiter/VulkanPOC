find_package(Vulkan REQUIRED)
add_library(sdl2 INTERFACE)
add_library(vkbootstrap STATIC)
add_library(vma INTERFACE)

set(sdl2_DIR "SDL_PATH" CACHE PATH "Path to SDL2")

target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include/SDL2 ${sdl2_image_DIR}/include/SDL2)
target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib ${sdl2_image_DIR}/lib)
target_link_libraries(sdl2 INTERFACE SDL2 SDL2main)

target_sources(vkbootstrap PRIVATE 
      vkbootstrap/VkBootstrap.h
      vkbootstrap/VkBootstrap.cpp
)
target_include_directories(vma INTERFACE vma)
target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)
