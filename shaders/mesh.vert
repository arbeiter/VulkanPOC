#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout( set = 0, binding = 0) uniform CameraBuffer
{
  mat4 view;
  mat4 pro;
  mat4 viewPro;
} cameraData;

layout (location = 0) out vec3 outColor;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() 
{
  outColor = vColor;
  gl_Position = uboMatrices.projection * uboMatrices.view * uboMatrices.model * vec4(vPosition.xyz, 1.0);
}
