### TODO:
// Use descriptors to send mesh matrix to the shader in draw_object
// Research vkCmdBindVertexBuffers
// Figure out memory leak

### Current change
Init Scene

#### Passing projection data in with uniform buffer

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

void VulkanEngine::load_meshes()
{
  Mesh triMesh{};
  //make the array 3 vertices long
  triMesh._vertices.resize(3);

  //vertex positions
  triMesh._vertices[0].position = { 1.f,1.f, 0.0f };
  triMesh._vertices[1].position = { -1.f,1.f, 0.0f };
  triMesh._vertices[2].position = { 0.f,-1.f, 0.0f };

  //vertex colors, all green
  triMesh._vertices[0].color = { 0.f,1.f, 0.0f }; //pure green
  triMesh._vertices[1].color = { 0.f,1.f, 0.0f }; //pure green
  triMesh._vertices[2].color = { 0.f,1.f, 0.0f }; //pure green
  //we dont care about the vertex normals

  //load the monkey
  Mesh monkeyMesh{};
  monkeyMesh.load_from_obj("../../assets/monkey_smooth.obj");

  upload_mesh(triMesh);
  upload_mesh(monkeyMesh);

  _meshes["monkey"] = monkeyMesh;
  _meshes["triangle"] = triMesh;
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

#### init_descriptors

Already set up for camera mvp matrices

#### push_constants
```

layout( push_constant ) uniform constants
{
vec4 data;
mat4 render_matrix;
} PushConstants;

void main() 
{
  mat4 model = PushConstants.render_matrix;
  gl_Position = model * vec4(vPosition, 1.0f);  
}

{
    RenderObject& object = first[i];

    //only bind the pipeline if it doesnt match
with the already bound one
    if (object.material != lastMaterial) {
        vkCmdBindPipeline(cmd,
  VK_PIPELINE_BIND_POINT_GRAPHICS,
  object.material->pipeline);
        lastMaterial = object.material;
    }

    glm::mat4 model = object.transformMatrix;
    //final render matrix, that we are calculating
on the cpu
    glm::mat4 mesh_matrix = projection * view *
model;

    MeshPushConstants constants;
    constants.render_matrix = mesh_matrix;

    //upload the mesh to the gpu via pushconstants
    vkCmdPushConstants(cmd,
object.material->pipelineLayout,
VK_SHADER_STAGE_VERTEX_BIT, 0,
sizeof(MeshPushConstants), &constants);

    //only bind the mesh if its a different one
from last bind
    if (object.mesh != lastMesh) {
      //bind the mesh vertex buffer with offset 0
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1,
&object.mesh->_vertexBuffer._buffer, &offset);
      lastMesh = object.mesh;
  }
}

```
