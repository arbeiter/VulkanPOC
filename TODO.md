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
