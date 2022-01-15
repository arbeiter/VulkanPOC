# Add source to this project's executable.
add_executable(SDLTestApp
    main.cpp
    vk_engine.cpp
    vk_engine.h
    vk_types.h
    vk_initializers.cpp
    vk_initializers.h
    vk_mesh.h
    vk_mesh.cpp
    vk_textures.cpp
    vk_textures.h)

set_property(TARGET SDLTestApp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:SDLTestApp>")

target_include_directories(SDLTestApp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(SDLTestApp vkbootstrap vma glm tinyobjloader stb_image)
target_link_libraries(SDLTestApp Vulkan::Vulkan sdl2)

add_dependencies(SDLTestApp Shaders)
