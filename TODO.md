### Todo:

- Frame dependence
- Scene data -> dynamic descriptors
- Camera data

### Current

#### Experiment 2: Triangle upload with scenes

Notes:
  - Study current code to figure out how to modify draw_objects
  - You might be able to use uniform buffers to map data into the descriptor for model data, if not use SSBOs
  - In the worst case, you can use push constants but I'd try to avoid that

1.  Add renderables
```
Mesh: 
  getMesh, create_mesh
Material: pipeline, pipelineLayout
  getMaterial, create_material

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

##### Adding an SSBO for the model matrix

https://github.com/David-DiGioia/monet/blob/7bd5becacbdcdca40f48fedb9098d38e47f31ff0/src/vk_engine.cpp

Storage Buffers:
  1. Create shader storage buffer:
    a. inside init descriptors -> create storage buffer per frame 
  2. New descriptor set:
    a. add AllocatedBuffer objectBuffer, VkDescriptorSet objectDescriptor to frame data
    b. Allocate a VK_DESCRIPTOR_TYPE_STORAGE_BUFFER DescriptorPool pool
    c. Initialize VkDescriptorSetLayoutBinding 
    d. vkCreateDescriptorSetLayout with DescriptorSetLayoutInfo's binding count = 1: objectSetLayout
  3. Create descriptor sets to point to the buffer:
    a. VkDescriptorSetAllocateInfo objectSetAlloc points to descriptorPool and objectSetLayout
    AllocateDescriptorSets with objectSetAlloc
    b. VkDescriptorBufferInfo objectBufferInfo points to objectBuffer._buffer
    b2. Inside initDescriptorSets:
    ```
      objectWrite = write_descriptor_buffer
      setWRites = { camera, scene, object }
      updateDescriptorSets(_device, 3, setWrites, 0, nullptr);
    ```
    c. Add this to the shader:
    ```	
      layout(std140,set = 1, binding = 0) readonly buffer ObjectBuffer{
        ObjectData objects[];
      } objectBuffer;
      mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
    ```
    d. Add to list of descriptors in init_pipelines: VkDescriptorSetLayout: objectSetLayout
  4. DrawObjects(called with: RenderObject first)
      Each renderable has a mesh, material, transformObject
      Each GPUObject has a model matrix

      1. Set up camData(uniform), sceneData(dynamic uniform), objectData(SSBO)
      2. Copy scene data and cam data into buffers
      3. Write object's matrices into SSBO using GPUObjectData. Each frame has an objectBuffer
        a. Map memory for current frame's object buffer using vmaMapMemory
        b. For each renderable:
          i. objectSSBO[id].modelMatrtix = renderable.transformMatrix
      4. For each renderable:
        a. Material
          ```
          vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                  object.material->pipelineLayout, 1, 1, 
                                  &get_current_frame().objectDescriptor, 0, nullptr);
          ```
        b. Mesh:
          ```
          glm::mat4 model = object.transformMatrix;
          ```
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
