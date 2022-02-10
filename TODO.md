### Todo:

- Frame dependence
- Scene data -> dynamic descriptors
- Camera data

### Current




#### Experiment 2: Triangle upload with scenes

1.  Add renderables
```
Mesh: 
  getMesh, create_mesh
Material: pipeline, pipelineLayout
  getMaterial, create_material
struct RenderObject {
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;
};

struct Material {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

std::vector<RenderObject> _renderables;
std::unordered_map<std::string, Material> _materials;
std::unordered_map<std::string, Mesh> _meshes;

void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);
draw_objects(cmd, _renderables.data(), _renderables.size());
void init_scene();
```

2. Mesh uploads
```
get_mesh
get_material
```

3. InitScene:
```
For 1 triangle:
  tri.mesh, tri.material, translation, scale, transformMatrix
  renderables.push_back(tri)
```

4. Draw Objects:
```
camPos, view, projection
lastMesh, lastMaterial
for(i = 0; i < count; i++) {
  bindPipeline(material->pipeline)
  setup model
  setup renderMatrix
  bindVertexBuffers
  Draw(vertices)
}
```

##### Additional notes
https://github.com/David-DiGioia/monet/blob/7bd5becacbdcdca40f48fedb9098d38e47f31ff0/src/vk_engine.cpp

Storage Buffers:
  1. Create shader storage buffer:
    a. inside init descriptors -> create buffer per frame 
  2. New descriptor set:
    a. add objectBuffer, objectDescriptor to frame data
    b. Allocate a VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
    c. Initialize VkDescriptorSetLayoutBinding
    d. vkCreateDescriptorSetLayout
  3. Create descriptor sets to point to the buffer:
    a. VkDescriptorSetAllocateInfo objectSetAlloc = {}; 
    b. VkDescriptorBufferInfo objectBufferInfo; -> points to objectBuffer._buffer
    c. Add this to the shader:
    ```	
      layout(std140,set = 1, binding = 0) readonly buffer ObjectBuffer{

        ObjectData objects[];
      } objectBuffer;
      mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
    ```
    d. Add to list of descriptors in init_pipelines: VkDescriptorSetLayout setLayouts
  4. DrawObjects
    
DrawObjects ->
  Each renderable has a mesh, material, transformObject
  Each GPUObject has a model matrix
  1. camData, sceneData, objectData
  2. Copy scene data and cam data into buffers
  3. Write object's matrices into SSBO using GPUObjectData. Each frame has an objectBuffer
    a. Map memory for current frame's object buffer
    b. For each renderable:
      i. objectSSBO[id].modelMatrtix = renderable.transformMatrix
  https://vkguide.dev/docs/chapter-4/storage_buffers/

#### Experiment 1: Triangle Upload Attempt
1.
```
LoadMeshes
	//make the array 3 vertices long
	_triangleMesh._vertices.resize(3);

	//vertex positions
	_triangleMesh._vertices[0].position = { 1.f,1.f, 0.0f };
	_triangleMesh._vertices[1].position = { -1.f,1.f, 0.0f };
	_triangleMesh._vertices[2].position = { 0.f,-1.f, 0.0f };

	//vertex colors, all green
	_triangleMesh._vertices[0].color = { 0.f,1.f, 0.0f }; //pure green
	_triangleMesh._vertices[1].color = { 0.f,1.f, 0.0f }; //pure green
	_triangleMesh._vertices[2].color = { 0.f,1.f, 0.0f }; //pure green
	//we dont care about the vertex normals
	upload_mesh(_triangleMesh);
```

2. 
```
Draw:

	//other code ....
	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);

	//bind the mesh vertex buffer with offset 0
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &_triangleMesh._vertexBuffer._buffer, &offset);

	//we can now draw the mesh
	vkCmdDraw(cmd, _triangleMesh._vertices.size(), 1, 0, 0);

	//finalize the render pass
	vkCmdEndRenderPass(cmd);
```

3. 
```
Pipeline builder

	//build the mesh pipeline
	VertexInputDescription vertexDescription = Vertex::get_vertex_description();

	//connect the pipeline builder vertex input info to the one we get from Vertex
	pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

	pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

	//clear the shader stages for the builder
	pipelineBuilder._shaderStages.clear();
```

Result:
A rotating triangle showed up. 
Change logic to drawScene for setting up _renderables

```
Create a list of vertices 
Create a vertex buffer

VkMesh:
  Vertices -> pos, normal, color, uv
  AllocatedBuffer

Upload Mesh
  Allocate vertex buffer
  Use VMA to create the buffer

Set the vertex input layout in the pipeline
Init_Pipelines:
  Build mesh pipeline
  Compile Mesh Vertex Shader
  Add the other shaders
 
Draw:
  CmdBeginRenderPass 
  CmdBindPipeline
  BindVertexBuffers
  CmdDraw
  CmdEndRenderPass

```
