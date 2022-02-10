### Current

#### Ways of uploading data to the GPU

1. Obj: Load obj, upload to the GPU using descriptor sets, simple shading.
2. Generated Mesh with hardcoded vertices and index buffer(Current)

#### Generated Mesh upload plan:

Use static descriptor sets for this

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

#### Code change summary
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
