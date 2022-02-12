### TODO:
// Use descriptors to send mesh matrix to the shader in draw_object
// Research vkCmdBindVertexBuffers
// Figure out memory leak

### Current change
Init Scene

#### vk_engine.cpp

```
struct RenderObject {
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;
};

Material* VulkanEngine::create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name)
{
	Material mat;
	mat.pipeline = pipeline;
	mat.pipelineLayout = layout;
	_materials[name] = mat;
	return &_materials[name];
}

Material* VulkanEngine::get_material(const std::string& name)
{
	//search for the object, and return nullpointer if not found
	auto it = _materials.find(name);
	if (it == _materials.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}

Mesh* VulkanEngine::get_mesh(const std::string& name)
{
	auto it = _meshes.find(name);
	if (it == _meshes.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}

void VulkanEngine::init_scene() {
	RenderObject cube;
	cube.mesh = get_mesh("cube");
	cube.material = get_material("defaultmesh");
	cube.transformMatrix = glm::mat4{ 1.0f };
	_renderables.push_back(cube);

  RenderObject tri;
  tri.mesh = get_mesh("triangle");
  tri.material = get_material("defaultmesh");
  glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
  glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
  tri.transformMatrix = translation * scale;

  _renderables.push_back(tri);
}

void VulkanEngine::draw_objects(VkCommandBuffer cmd,RenderObject* first, int count) {
  // Make a model view matrix for rendering the object
  // Camera View -> translate a vec3 camera pos
  // Camera Projection -> glm::perspective
  // for each renderable:
    // bind pipeline with object's materials pipeline
object.material->pipeline)
  // mesh_matrix = proj * view * object.transformMatrix
  // get the vertexBuffer.buffer of object.mesh
  // bind it using vkCmdBindVertexBuffers
  // vkCmdDraw(cmd, object.mesh->-vertices.size())
}

void draw() {
	draw_objects(cmd, _renderables.data(), _renderables.size());
}


```
#### vk_engine.h
```
struct Material {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

std::vector<RenderObject> _renderables;
std::unordered_map<std::string, Material> _materials;
std::unordered_map<std::string, Mesh> _meshes;

void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);
void init_scene();

```
