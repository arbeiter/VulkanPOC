// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.
#pragma once

#include <vk_types.h>
#include <vector>
#include <functional>
#include <deque>

struct DeletionQueue {
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call the function
		}

		deletors.clear();
	}
};

class PipelineBuilder {
public:

	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkViewport _viewport;
	VkRect2D _scissor;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;
	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};

class VulkanEngine {
public:
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;
  VkRenderPass _renderPass;
  std::vector<VkFramebuffer> _framebuffers;

  VkSurfaceKHR _surface;
  VkSwapchainKHR _swapchain;
  VkFormat _swapchainImageFormat;
  std::vector<VkImage> _swapchainImages;
  std::vector<VkImageView> _swapchainImageViews;

	VkPipelineLayout _trianglePipelineLayout;
	VkPipeline _trianglePipeline;
  VkPipeline _redTrianglePipeline;

	VkQueue _graphicsQueue; //queue we will submit to
	uint32_t _graphicsQueueFamily; //family of that queue

	VkCommandPool _commandPool; //the command pool for our commands
	VkCommandBuffer _mainCommandBuffer; //the buffer we will record into

	DeletionQueue _mainDeletionQueue;
  VmaAllocator _allocator; //vma lib allocator

	bool _isInitialized{ false };
	int _frameNumber {0};
	int _selectedShader {0};

	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

private:
  void init_commands();
	void init_vulkan();
  void init_swapchain();
  void init_default_renderpass();
	void init_pipelines();
  void init_framebuffers();
	void init_sync_structures();
  bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);
};
