// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.
#pragma once

#include <vk_types.h>
#include <string>
#include <vector>
#include <functional>
#include <deque>
#include <vk_mesh.h>
#include <glm/glm.hpp>


//number of frames to overlap when rendering
constexpr unsigned int FRAME_OVERLAP = 2;

struct GPUCameraData{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};

struct FrameData {
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;	

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	//buffer that holds a single GPUCameraData to use when rendering
	AllocatedBuffer cameraBuffer;

	VkDescriptorSet globalDescriptor;
};

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct Material {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject {
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;
};

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
	VkPipelineDepthStencilStateCreateInfo _depthStencil;
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
  VkDescriptorSetLayout _globalSetLayout;
  VkDescriptorPool _descriptorPool;

  //frame storage
  FrameData _frames[FRAME_OVERLAP];

  //getter for the frame we are rendering to right now.
  FrameData& get_current_frame();

	//default array of renderable objects
	std::unordered_map<std::string,Material> _materials;
	std::unordered_map<std::string,Mesh> _meshes;
	std::vector<RenderObject> _renderables;
  Mesh _monkey_mesh;

  VkImageView _depthImageView;
  AllocatedMemory _depthImage;
  VkFormat _depthFormat;

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
  VkPipeline _meshPipeline;
  VkPipelineLayout _meshPipelineLayout;
  Mesh _triangleMesh;

	VkQueue _graphicsQueue; //queue we will submit to
	uint32_t _graphicsQueueFamily; //family of that queue

	DeletionQueue _mainDeletionQueue;
  VmaAllocator _allocator; //vma lib allocator

	bool _isInitialized{ false };
	int _frameNumber {0};
	int _selectedShader {0};

	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

  void init_scene();

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

	//create material and add it to the map
	Material* create_material(VkPipeline pipeline, VkPipelineLayout layout,const std::string& name);

	//returns nullptr if it can't be found
	Material* get_material(const std::string& name);

	//returns nullptr if it can't be found
	Mesh* get_mesh(const std::string& name);

	//our draw function
	void draw_objects(VkCommandBuffer cmd,RenderObject* first, int count);

  AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

private:
  void init_commands();
	void init_vulkan();
  void init_swapchain();
  void init_default_renderpass();
	void init_pipelines();
  void init_framebuffers();
	void init_sync_structures();
  void init_descriptors();
  bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);
  void load_meshes();
  void upload_mesh(Mesh& mesh);
};
