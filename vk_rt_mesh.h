// TODO: not like this
typedef HMM_Vec3 v3;

typedef struct {
  v3 pos;
  v3 norm;
  float uv[2];
} vkrt_vertex_t;

#include "vk_rt_help.h"

// TODO: BAD
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// temporary
typedef struct {
  float color[3];
  uint32_t texture_index;
} vkrt_material;

typedef struct {
  vkrt_memory vertex_buffer;
  vkrt_memory index_buffer;

  uint32_t vertex_count;
  uint32_t primitive_count;

  uint32_t material_index;
} vkrt_primitive;

typedef struct {
  size_t primitive_count;
  vkrt_primitive *primitives;
  vkrt_memory transform_buffer;
} vkrt_mesh;


typedef struct {
  size_t mesh_count;
  vkrt_mesh *meshes;
  size_t texture_count;
  vkw_image *textures;
  vkrt_memory materials_buffer;
} vkrt_model;

// NOTE HACK REMOVE THIS
static int count_zero_uvs = 0;

vkrt_primitive vkrt_load_gltf_primitive(VkDevice device, VmaAllocator allocator,
					cgltf_primitive p, cgltf_data *data) {
  // get the number of indices
  size_t index_count = cgltf_accessor_unpack_indices(p.indices, NULL, 1, 1);  
  // get the number of vertices in the primitive
  size_t vertex_count = 0;
  for (size_t i = 0; i < p.attributes_count; ++i) {
    if (p.attributes[i].type == cgltf_attribute_type_position) {
      vertex_count += p.attributes[i].data->count;
    }
  }
  uint32_t *indices = calloc(sizeof(*indices), index_count);
  vkrt_vertex_t *vertices = calloc(sizeof(*vertices), vertex_count);
  assert(indices && vertices); // TODO: proper error check or something

  cgltf_accessor_unpack_indices(p.indices, indices, 4, p.indices->count);

  for (size_t i = 0; i < p.attributes_count; ++i) {
    cgltf_accessor *attr = p.attributes[i].data;
    if (p.attributes[i].type == cgltf_attribute_type_position) {
      float *p_positions = (float *)attr->buffer_view->buffer->data +
	attr->buffer_view->offset/sizeof(float) + attr->offset/sizeof(float);
      for (size_t j = 0; j < attr->count; ++j) {
	vertices[j].pos = (v3) {
	  p_positions[3*j + 0],
	  p_positions[3*j + 1],
	  p_positions[3*j + 2],
	};
      }
    } else if (p.attributes[i].type == cgltf_attribute_type_normal) {
      float *p_normals = (float *)attr->buffer_view->buffer->data +
	attr->buffer_view->offset/sizeof(float) + attr->offset/sizeof(float);
      for (size_t j = 0; j < attr->count; ++j) {
	vertices[j].norm = (v3) {
	  p_normals[3*j + 0],
	  p_normals[3*j + 1],
	  p_normals[3*j + 2],
	};
      }
    } else if (p.attributes[i].type == cgltf_attribute_type_texcoord) {
      float *p_uvs = (float *)attr->buffer_view->buffer->data +
	attr->buffer_view->offset/sizeof(float) + attr->offset/sizeof(float);
      for (size_t j = 0; j < attr->count; ++j) {
	vertices[j].uv[0] = p_uvs[2*j];
	vertices[j].uv[1] = p_uvs[2*j + 1];
      }
    }
  }

  VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  
  vkrt_memory vertex_buffer =
    vkrt_allocate_memory(device, allocator, vertex_count * sizeof(vkrt_vertex_t),
			 vertices, usage);
  vkrt_memory index_buffer =
    vkrt_allocate_memory(device, allocator, index_count * sizeof(uint32_t),
			 indices, usage);
  
  free(vertices);
  free(indices);
  size_t material_idx = 0;
  if (p.material) {
    material_idx = cgltf_material_index(data, p.material);
  }
  
  return (vkrt_primitive) {
    .vertex_buffer = vertex_buffer,
    .index_buffer = index_buffer,
    .material_index = material_idx,
    .vertex_count = vertex_count,
    .primitive_count = index_count / 3
  };
}

vkrt_mesh
vkrt_load_gltf_mesh(VkDevice device, VmaAllocator allocator, cgltf_mesh mesh,
		    cgltf_node node, cgltf_data *data) {
  vkrt_mesh res = {
    .primitive_count = mesh.primitives_count,
    .primitives = calloc(sizeof(vkrt_primitive), mesh.primitives_count),
  };
  VkTransformMatrixKHR transform = {};
  HMM_Mat4 transform4 = {};
  cgltf_node_transform_world(&node, (float*)transform4.Elements);
  transform4 = HMM_TransposeM4(transform4);
  memcpy(&transform, &transform4.Elements, 12 * sizeof(float));

  VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  
  res.transform_buffer = vkrt_allocate_memory(device, allocator, sizeof(transform),
					      &transform, usage);
  
  for (size_t i = 0; i < mesh.primitives_count; ++i) {
    res.primitives[i] = vkrt_load_gltf_primitive(device, allocator, mesh.primitives[i], data);
  }

  return res;
}

vkrt_model
vkrt_load_gltf_model(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
		     vkw_immediate_submit_buffer immediate, const char *fp) {
  cgltf_options options = {};
  cgltf_data *data = NULL;
  cgltf_result res = cgltf_parse_file(&options, fp, &data);
  if (res != cgltf_result_success) {
    fprintf(stderr, "Failed to open gltf file %s (code: %d)\n", fp, res);
    exit(1);
  }
  res = cgltf_load_buffers(&options, data, fp);
  if (res != cgltf_result_success) {
    fprintf(stderr, "Failed to load gltf file %s (code: %d)\n", fp, res);
    exit(1);
  }
  
  vkrt_model model = { .mesh_count = data->meshes_count };
  model.meshes = calloc(sizeof(*model.meshes), data->meshes_count);
  
  // TODO: INCOMPLETE (need to populate materials and textures)
  vkrt_material *materials = calloc(sizeof(*materials), data->materials_count);
  size_t material_count = data->materials_count;
  for (size_t i = 0; i < material_count; ++i) {
    cgltf_material matt = data->materials[i];
    cgltf_pbr_metallic_roughness mat = matt.pbr_metallic_roughness;
    materials[i].color[0] = mat.base_color_factor[0];
    materials[i].color[1] = mat.base_color_factor[1];
    materials[i].color[2] = mat.base_color_factor[2];
    // TODO:
    if (mat.base_color_texture.texture) {
      materials[i].texture_index = cgltf_texture_index(data, mat.base_color_texture.texture);
    } else {
      // HACK TODO: this is not a real value and could contain a valid texture
      // in more complex scenes
      materials[i].texture_index = 256;
      printf("material at index %lu has no texture\n", i);
    }
  }

  model.texture_count = data->textures_count;
  model.textures = calloc(sizeof(*model.textures), model.texture_count);
  for (size_t i = 0; i < data->textures_count; ++i) {
    cgltf_texture tex = data->textures[i];
    // TODO: if the file is a gltf, then the images need to be loaded via their URI
    // which is relative to the path of the gltf model, so some work reconstructing
    // the location relative to the exe is needed
    if (tex.image && tex.image->buffer_view) {
      cgltf_buffer_view *view = tex.image->buffer_view;      
      int w, h, c;
      uint8_t *data = stbi_load_from_memory(view->buffer->data + view->offset, view->size,
					    &w, &h, &c, 4);
      VkExtent3D dims = { w, h, 1 };
      VkImageUsageFlagBits usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      model.textures[i] =
	vkw_image_create_data(device, allocator, immediate, scratch_queue, dims,
			      VK_FORMAT_R8G8B8A8_UNORM, usage, false, data);
      printf("Loaded image with dimensions: %d %d %d\n", w, h, 4);
      free(data);
    } else {
      fprintf(stderr, "Failed to load texture at index %lu\n", i);
      if (tex.image->uri) {
	fprintf(stderr, "This image should have been loaded from %s\n", tex.image->uri);
      }
      exit(1);
    }
  }

  VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  
  model.materials_buffer =
    vkrt_allocate_memory(device, allocator, sizeof(vkrt_material) * material_count,
			 materials, usage);

  free(materials);
  
  for (size_t i = 0; i < model.mesh_count; ++i) {
    model.meshes[i] = vkrt_load_gltf_mesh(device, allocator, data->meshes[i],
					  data->nodes[i], data);
  }

  cgltf_free(data);

  printf("%lu zero uvs\n", count_zero_uvs);
  
  return model;
}

void vkrt_free_model(VkDevice device, VmaAllocator allocator, vkrt_model model) {
  for (size_t i = 0; i < model.mesh_count; ++i) {
    vkrt_mesh mesh = model.meshes[i];
    for (size_t j = 0; j < mesh.primitive_count; ++j) {
      vkrt_primitive p = mesh.primitives[j];
      vkrt_memory_free(allocator, p.vertex_buffer);
      vkrt_memory_free(allocator, p.index_buffer);
    }
    vkrt_memory_free(allocator, mesh.transform_buffer);
    free(mesh.primitives);
  }
  for (size_t i = 0; i < model.texture_count; ++i) {
    vkw_image_destroy(device, allocator, model.textures[i]);
  }
  vkrt_memory_free(allocator, model.materials_buffer);
  free(model.textures);
  free(model.meshes);
}
