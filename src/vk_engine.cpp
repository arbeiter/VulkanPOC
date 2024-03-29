﻿/* TODO: Destroy descriptors and descriptor set layout
 * TODO: Bind descriptor set layout
 * TODO: Push model, view, projection changes
 * TODO: Add shader changes
 */
#include "vk_engine.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>

#include "VkBootstrap.h"

#include <iostream>
#include <fstream>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

constexpr bool bUseValidationLayers = true;

//we want to immediately abort when there is an error. In normal engines this would give an error message to the user, or perform a dump of state.
using namespace std;
#define VK_CHECK(x)                                                 \
  do                                                              \
{                                                               \
  VkResult err = x;                                           \
  if (err)                                                    \
  {                                                           \
    std::cout <<"Detected Vulkan error: " << err << std::endl; \
    abort();                                                \
  }                                                           \
} while (0)

void VulkanEngine::init()
{
  // We initialize SDL and create a window with it. 
  SDL_Init(SDL_INIT_VIDEO);

  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

  _window = SDL_CreateWindow(
      "Vulkan Engine",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      _windowExtent.width,
      _windowExtent.height,
      window_flags
      );

  init_vulkan();

  init_swapchain();

  init_default_renderpass();

  init_framebuffers();

  init_commands();

  init_sync_structures();

  setupDescriptors();

  init_pipelines();

  load_meshes();

  //everything went fine
  _isInitialized = true;
}
void VulkanEngine::cleanup()
{	
  if (_isInitialized) {

    //make sure the gpu has stopped doing its things
    vkDeviceWaitIdle(_device);
    _mainDeletionQueue.flush();
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);
    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
    vkDestroyInstance(_instance, nullptr);
    SDL_DestroyWindow(_window);
  }
}

void VulkanEngine::draw()
{
  VK_CHECK(vkWaitForFences(_device, 1, &_renderFence, true, 1000000000));
  VK_CHECK(vkResetFences(_device, 1, &_renderFence));
  VK_CHECK(vkResetCommandBuffer(_mainCommandBuffer, 0));
  uint32_t swapchainImageIndex;
  VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _presentSemaphore, nullptr, &swapchainImageIndex));
  VkCommandBuffer cmd = _mainCommandBuffer;
  VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
  VkClearValue clearValue;
  float flash = abs(sin(_frameNumber / 120.f));
  clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };
  VkClearValue depthClear;
  depthClear.depthStencil.depth = 1.f;
  VkRenderPassBeginInfo rpInfo = vkinit::renderpass_begin_info(_renderPass, _windowExtent, _framebuffers[swapchainImageIndex]);
  rpInfo.clearValueCount = 2;
  VkClearValue clearValues[] = { clearValue, depthClear };
  rpInfo.pClearValues = &clearValues[0];
  vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

  // Remove this code with drawObjects
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 0, 1, &descriptorSet, 0, nullptr); 
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(cmd, 0, 1, &_monkeyMesh._vertexBuffer._buffer, &offset);
  glm::vec3 camPos = { 0.f, 0.f, -20.f };
  glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
  glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
  projection[1][1] *= -1;
  glm::mat4 model = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_frameNumber * 0.4f), glm::vec3(0, 1, 0));
  GPUCameraData camData;
  camData.projection = projection;
  camData.view = view;
  camData.model = model;
  void *data;
  vmaMapMemory(_allocator, cameraBuffer._allocation, &data);
  memcpy(data, &camData, sizeof(GPUCameraData));
  vmaUnmapMemory(_allocator, cameraBuffer._allocation);
  vkCmdDraw(cmd, _monkeyMesh._vertices.size(), 1, 0, 0);
  // End block

  vkCmdEndRenderPass(cmd);
  VK_CHECK(vkEndCommandBuffer(cmd));

  VkSubmitInfo submit = vkinit::submit_info(&cmd);
  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submit.pWaitDstStageMask = &waitStage;
  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = &_presentSemaphore;
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = &_renderSemaphore;
  VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _renderFence));
  VkPresentInfoKHR presentInfo = vkinit::present_info();
  presentInfo.pSwapchains = &_swapchain;
  presentInfo.swapchainCount = 1;
  presentInfo.pWaitSemaphores = &_renderSemaphore;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pImageIndices = &swapchainImageIndex;
  VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));
  _frameNumber++;
}

void VulkanEngine::run()
{
  SDL_Event e;
  bool bQuit = false;

  //main loop
  while (!bQuit)
  {
    //Handle events on queue
    while (SDL_PollEvent(&e) != 0)
    {
      //close the window when user alt-f4s or clicks the X button			
      if (e.type == SDL_QUIT)
      {
        bQuit = true;
      }
    }
    draw();
  }
}

void VulkanEngine::init_vulkan()
{
  vkb::InstanceBuilder builder;

  //make the vulkan instance, with basic debug features
  auto inst_ret = builder.set_app_name("Example Vulkan Application")
    .request_validation_layers(bUseValidationLayers)
    .use_default_debug_messenger()
    .require_api_version(1, 1, 0)
    .build();

  vkb::Instance vkb_inst = inst_ret.value();

  //grab the instance 
  _instance = vkb_inst.instance;
  _debug_messenger = vkb_inst.debug_messenger;

  SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

  //use vkbootstrap to select a gpu. 
  //We want a gpu that can write to the SDL surface and supports vulkan 1.2
  vkb::PhysicalDeviceSelector selector{ vkb_inst };
  vkb::PhysicalDevice physicalDevice = selector
    .set_minimum_version(1, 1)
    .set_surface(_surface)
    .select()
    .value();

  //create the final vulkan device

  vkb::DeviceBuilder deviceBuilder{ physicalDevice };

  vkb::Device vkbDevice = deviceBuilder.build().value();

  // Get the VkDevice handle used in the rest of a vulkan application
  _device = vkbDevice.device;
  _chosenGPU = physicalDevice.physical_device;

  // use vkbootstrap to get a Graphics queue
  _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();

  _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

  //initialize the memory allocator
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = _chosenGPU;
  allocatorInfo.device = _device;
  allocatorInfo.instance = _instance;
  vmaCreateAllocator(&allocatorInfo, &_allocator);

  _mainDeletionQueue.push_function([&]() {
      vmaDestroyAllocator(_allocator);
      });
}

void VulkanEngine::init_swapchain()
{
  vkb::SwapchainBuilder swapchainBuilder{_chosenGPU,_device,_surface };

  vkb::Swapchain vkbSwapchain = swapchainBuilder
    .use_default_format_selection()
    //use vsync present mode
    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
    .set_desired_extent(_windowExtent.width, _windowExtent.height)
    .build()
    .value();

  //store swapchain and its related images
  _swapchain = vkbSwapchain.swapchain;
  _swapchainImages = vkbSwapchain.get_images().value();
  _swapchainImageViews = vkbSwapchain.get_image_views().value();

  _swachainImageFormat = vkbSwapchain.image_format;

  _mainDeletionQueue.push_function([=]() {
      vkDestroySwapchainKHR(_device, _swapchain, nullptr);
      });

  //depth image size will match the window
  VkExtent3D depthImageExtent = {
    _windowExtent.width,
    _windowExtent.height,
    1
  };

  //hardcoding the depth format to 32 bit float
  _depthFormat = VK_FORMAT_D32_SFLOAT;

  //the depth image will be a image with the format we selected and Depth Attachment usage flag
  VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

  //for the depth image, we want to allocate it from gpu local memory
  VmaAllocationCreateInfo dimg_allocinfo = {};
  dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  //allocate and create the image
  vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depthImage._image, &_depthImage._allocation, nullptr);

  //build a image-view for the depth image to use for rendering
  VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);;

  VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImageView));

  //add to deletion queues
  _mainDeletionQueue.push_function([=]() {
      vkDestroyImageView(_device, _depthImageView, nullptr);
      vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);
      });
}

void VulkanEngine::init_default_renderpass()
{
  //we define an attachment description for our main color image
  //the attachment is loaded as "clear" when renderpass start
  //the attachment is stored when renderpass ends
  //the attachment layout starts as "undefined", and transitions to "Present" so its possible to display it
  //we dont care about stencil, and dont use multisampling

  VkAttachmentDescription color_attachment = {};
  color_attachment.format = _swachainImageFormat;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref = {};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depth_attachment = {};
  // Depth attachment
  depth_attachment.flags = 0;
  depth_attachment.format = _depthFormat;
  depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref = {};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  //we are going to create 1 subpass, which is the minimum you can do
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;
  //hook the depth attachment into the subpass
  subpass.pDepthStencilAttachment = &depth_attachment_ref;

  //1 dependency, which is from "outside" into the subpass. And we can read or write color
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  //dependency from outside to the subpass, making this subpass dependent on the previous renderpasses
  VkSubpassDependency depth_dependency = {};
  depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  depth_dependency.dstSubpass = 0;
  depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depth_dependency.srcAccessMask = 0;
  depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  //array of 2 dependencies, one for color, two for depth
  VkSubpassDependency dependencies[2] = { dependency, depth_dependency };

  //array of 2 attachments, one for the color, and other for depth
  VkAttachmentDescription attachments[2] = { color_attachment,depth_attachment };

  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  //2 attachments from attachment array
  render_pass_info.attachmentCount = 2;
  render_pass_info.pAttachments = &attachments[0];
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  //2 dependencies from dependency array
  render_pass_info.dependencyCount = 2;
  render_pass_info.pDependencies = &dependencies[0];

  VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));

  _mainDeletionQueue.push_function([=]() {
      vkDestroyRenderPass(_device, _renderPass, nullptr);
      });
}

void VulkanEngine::init_framebuffers()
{
  //create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
  VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(_renderPass, _windowExtent);

  const uint32_t swapchain_imagecount = _swapchainImages.size();
  _framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

  for (int i = 0; i < swapchain_imagecount; i++) {

    VkImageView attachments[2];
    attachments[0] = _swapchainImageViews[i];
    attachments[1] = _depthImageView;

    fb_info.pAttachments = attachments;
    fb_info.attachmentCount = 2;
    VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));

    _mainDeletionQueue.push_function([=]() {
        vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
        vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
        });
  }
}

void VulkanEngine::init_commands()
{
  //create a command pool for commands submitted to the graphics queue.
  //we also want the pool to allow for resetting of individual command buffers
  VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool));

  //allocate the default command buffer that we will use for rendering
  VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_commandPool, 1);

  VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_mainCommandBuffer));

  _mainDeletionQueue.push_function([=]() {
      vkDestroyCommandPool(_device, _commandPool, nullptr);
      });
}

void VulkanEngine::init_sync_structures()
{
  //create syncronization structures
  //one fence to control when the gpu has finished rendering the frame,
  //and 2 semaphores to syncronize rendering with swapchain
  //we want the fence to start signalled so we can wait on it on the first frame
  VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

  VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_renderFence));

  //enqueue the destruction of the fence
  _mainDeletionQueue.push_function([=]() {
      vkDestroyFence(_device, _renderFence, nullptr);
      });
  VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

  VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_presentSemaphore));
  VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderSemaphore));

  //enqueue the destruction of semaphores
  _mainDeletionQueue.push_function([=]() {
      vkDestroySemaphore(_device, _presentSemaphore, nullptr);
      vkDestroySemaphore(_device, _renderSemaphore, nullptr);
      });
}

bool VulkanEngine::load_shader_module(const char* filePath, VkShaderModule* outShaderModule)
{
  //open the file. With cursor at the end
  std::ifstream file(filePath, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    return false;
  }

  //find what the size of the file is by looking up the location of the cursor
  //because the cursor is at the end, it gives the size directly in bytes
  size_t fileSize = (size_t)file.tellg();

  //spirv expects the buffer to be on uint32, so make sure to reserve a int vector big enough for the entire file
  std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

  //put file cursor at beggining
  file.seekg(0);

  //load the entire file into the buffer
  file.read((char*)buffer.data(), fileSize);

  //now that the file is loaded into the buffer, we can close it
  file.close();

  //create a new shader module, using the buffer we loaded
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = nullptr;

  //codeSize has to be in bytes, so multply the ints in the buffer by size of int to know the real size of the buffer
  createInfo.codeSize = buffer.size() * sizeof(uint32_t);
  createInfo.pCode = buffer.data();

  //check that the creation goes well.
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    return false;
  }
  *outShaderModule = shaderModule;
  return true;
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass)
{
  //make viewport state from our stored viewport and scissor.
  //at the moment we wont support multiple viewports or scissors
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.pNext = nullptr;

  viewportState.viewportCount = 1;
  viewportState.pViewports = &_viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &_scissor;

  //setup dummy color blending. We arent using transparent objects yet
  //the blending is just "no blend", but we do write to the color attachment
  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.pNext = nullptr;

  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &_colorBlendAttachment;

  //build the actual pipeline
  //we now use all of the info structs we have been writing into into this one to create the pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = nullptr;

  pipelineInfo.stageCount = _shaderStages.size();
  pipelineInfo.pStages = _shaderStages.data();
  pipelineInfo.pVertexInputState = &_vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &_inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &_rasterizer;
  pipelineInfo.pMultisampleState = &_multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDepthStencilState = &_depthStencil;
  pipelineInfo.layout = _pipelineLayout;
  pipelineInfo.renderPass = pass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  //its easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
  VkPipeline newPipeline;
  if (vkCreateGraphicsPipelines(
        device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
    std::cout << "failed to create pipline\n";
    return VK_NULL_HANDLE; // failed to create graphics pipeline
  }
  else
  {
    return newPipeline;
  }
}

void VulkanEngine::init_pipelines()
{
  //build the stage-create-info for both vertex and fragment stages. This lets the pipeline know the shader modules per stage
  PipelineBuilder pipelineBuilder;
  pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
  pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

  VertexInputDescription vertexDescription = Vertex::get_vertex_description();
  pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
  pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
  pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
  pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

  //build viewport and scissor from the swapchain extents
  pipelineBuilder._viewport.x = 0.0f;
  pipelineBuilder._viewport.y = 0.0f;
  pipelineBuilder._viewport.width = (float)_windowExtent.width;
  pipelineBuilder._viewport.height = (float)_windowExtent.height;
  pipelineBuilder._viewport.minDepth = 0.0f;
  pipelineBuilder._viewport.maxDepth = 1.0f;
  pipelineBuilder._scissor.offset = { 0, 0 };
  pipelineBuilder._scissor.extent = _windowExtent;
  pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
  pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
  pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();
  pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

  //clear the shader stages for the builder
  pipelineBuilder._shaderStages.clear();

  //compile mesh vertex shader
  VkShaderModule meshVertShader;
  load_shader_module("../shaders/mesh.vert.spv", &meshVertShader);

  VkShaderModule triangleFragShader;
  load_shader_module("../shaders/colored_triangle.frag.spv", &triangleFragShader);

  //add the other shaders
  pipelineBuilder._shaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
  pipelineBuilder._shaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

  VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipeline_layout_create_info();
  mesh_pipeline_layout_info.setLayoutCount = 1;
  mesh_pipeline_layout_info.pSetLayouts = &descriptorSetLayout;

  std::cout << "HERE";
  VK_CHECK(vkCreatePipelineLayout(_device, &mesh_pipeline_layout_info, nullptr, &_meshPipelineLayout));
  pipelineBuilder._pipelineLayout = _meshPipelineLayout;
  _meshPipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

  vkDestroyShaderModule(_device, triangleFragShader, nullptr);
  vkDestroyShaderModule(_device, meshVertShader, nullptr);
  _mainDeletionQueue.push_function([=]() {
      vkDestroyPipeline(_device, _meshPipeline, nullptr);
      vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr);
      });
}

AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
                           &newBuffer._buffer,
                           &newBuffer._allocation,
                           nullptr));

	return newBuffer;
}

void VulkanEngine::setupDescriptors()
{
  cameraBuffer = create_buffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  std::array<VkDescriptorSetLayoutBinding,1> setLayoutBindings{};
  setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  setLayoutBindings[0].binding = 0;
  setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  setLayoutBindings[0].descriptorCount = 1;

  VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
  descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
  descriptorLayoutCI.pBindings = setLayoutBindings.data();

  VK_CHECK(vkCreateDescriptorSetLayout(_device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));

  /* Descriptor Pool */
  std::array<VkDescriptorPoolSize, 1> descriptorPoolSizes{};
  descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorPoolSizes[0].descriptorCount = 1;
  VkDescriptorPoolCreateInfo descriptorPoolCI = {};
  descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
  descriptorPoolCI.pPoolSizes = descriptorPoolSizes.data();
  descriptorPoolCI.maxSets = 1;
  VK_CHECK(vkCreateDescriptorPool(_device, &descriptorPoolCI, nullptr, &descriptorPool));

  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = descriptorPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &descriptorSetLayout;
  VK_CHECK(vkAllocateDescriptorSets(_device, &allocateInfo, &descriptorSet));

  VkDescriptorBufferInfo binfo;
  binfo.buffer = cameraBuffer._buffer;
  binfo.offset = 0;
  binfo.range = sizeof(GPUCameraData);

  std::array<VkWriteDescriptorSet, 1> writeDescriptorSets{};
  writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSets[0].dstSet = descriptorSet;
  writeDescriptorSets[0].dstBinding = 0;
  writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeDescriptorSets[0].pBufferInfo = &binfo;
  writeDescriptorSets[0].descriptorCount = 1;

  vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

void VulkanEngine::load_meshes()
{
  _monkeyMesh.load_from_obj("../assets/cube.obj");
  upload_mesh(_monkeyMesh);
}

void VulkanEngine::upload_mesh(Mesh& mesh)
{
  //allocate vertex buffer
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.pNext = nullptr;
  bufferInfo.size = mesh._vertices.size() * sizeof(Vertex);
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  //let the VMA library know that this data should be writeable by CPU, but also readable by GPU
  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  //allocate the buffer
  VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
        &mesh._vertexBuffer._buffer,
        &mesh._vertexBuffer._allocation,
        nullptr));

  //add the destruction of triangle mesh buffer to the deletion queue
  _mainDeletionQueue.push_function([=]() {
      vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation);
      });

  //copy vertex data
  void* data;
  vmaMapMemory(_allocator, mesh._vertexBuffer._allocation, &data);
  memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
  vmaUnmapMemory(_allocator, mesh._vertexBuffer._allocation);
}
