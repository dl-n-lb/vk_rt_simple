#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 2) rayPayloadInEXT bool shadowed;

void main() {
  shadowed = false; // if the ray missed, that point isn't in shadow
}