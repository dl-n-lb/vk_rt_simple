#ifndef VK_RT_HELP_H_
#define VK_RT_HELP_H_
#include <assert.h>

#include "vk_mem_alloc.h"
#include "vulkan/vulkan.h"

// p appended to names since some of these symbols clash with function definitions
// even though there is no implementation associated with them...
PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHRp;
PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHRp;
PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHRp;
PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHRp;
PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHRp;
PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHRp;
PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHRp;
PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHRp;

#define VK_RESOLVE_DEVICE_PFN(device, pfn) \
  pfn##p = (PFN_##pfn)vkGetDeviceProcAddr(device, #pfn);

void vkrt_get_device_functions(VkDevice device) {
  VK_RESOLVE_DEVICE_PFN(device, vkGetAccelerationStructureBuildSizesKHR);
  VK_RESOLVE_DEVICE_PFN(device, vkCreateAccelerationStructureKHR);
  VK_RESOLVE_DEVICE_PFN(device, vkCmdBuildAccelerationStructuresKHR);  
  VK_RESOLVE_DEVICE_PFN(device, vkGetAccelerationStructureDeviceAddressKHR);
  VK_RESOLVE_DEVICE_PFN(device, vkCreateRayTracingPipelinesKHR);
  VK_RESOLVE_DEVICE_PFN(device, vkGetRayTracingShaderGroupHandlesKHR);
  VK_RESOLVE_DEVICE_PFN(device, vkCmdTraceRaysKHR);
  VK_RESOLVE_DEVICE_PFN(device, vkDestroyAccelerationStructureKHR);
}

typedef struct {
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo info;
  VkDeviceAddress device_address;
} vkrt_memory;

typedef vkrt_memory vkrt_as_memory;

vkrt_memory
vkrt_allocate_memory(VkDevice device, VmaAllocator allocator, uint64_t size,
		     void *data, VkBufferUsageFlagBits usage) {
  VkBufferCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
  };
  VmaAllocationCreateInfo vma_info = {
    .usage = VMA_MEMORY_USAGE_AUTO,
    .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
    | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
  };

  vkrt_memory res = {};
  VK_CHECK(vmaCreateBuffer(allocator, &info, &vma_info, &res.buffer, &res.allocation,
			   &res.info));
  VkBufferDeviceAddressInfo addr_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    .buffer = res.buffer,
  };
  res.device_address = vkGetBufferDeviceAddress(device, &addr_info);

  if (data != NULL) {
    VK_CHECK(vmaCopyMemoryToAllocation(allocator, data, res.allocation, 0, size));
  }

  return res;
}

void vkrt_memory_free(VmaAllocator allocator, vkrt_memory memory) {
  vmaDestroyBuffer(allocator, memory.buffer, memory.allocation);
}

typedef struct {
  VkAccelerationStructureKHR as;
  vkrt_as_memory memory;
  uint32_t handle;
} vkrt_as;

typedef enum {
  vkrt_as_bottom = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
  vkrt_as_top = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
} vkrt_as_level;

VkAccelerationStructureBuildGeometryInfoKHR
vkrt_as_build_geometry_info(vkrt_as_level lvl, uint64_t geom_cnt,
			    const VkAccelerationStructureGeometryKHR *geoms) {
  return (VkAccelerationStructureBuildGeometryInfoKHR) {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
    .type = lvl,
    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
    .geometryCount = geom_cnt,
    .pGeometries = geoms,
  };
}

vkrt_as vkrt_create_as(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
		       vkw_immediate_submit_buffer immediate, vkrt_as_level lvl,
		       VkAccelerationStructureBuildGeometryInfoKHR geom_info,
		       uint32_t primitive_count) {  
  vkrt_as as;
  VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
  };
  
  vkGetAccelerationStructureBuildSizesKHRp(device,
					   VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
					   &geom_info, &primitive_count,
					   &as_build_sizes_info); 
  as.memory =
    vkrt_allocate_memory(device, allocator,
			 as_build_sizes_info.accelerationStructureSize, NULL,
			 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
			 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  VkAccelerationStructureCreateInfoKHR as_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
    .buffer = as.memory.buffer,
    .size = as_build_sizes_info.accelerationStructureSize,
    .type = lvl,
  };

  VK_CHECK(vkCreateAccelerationStructureKHRp(device, &as_info, NULL, &as.as));
  
  vkrt_memory as_scratch_buffer =
    vkrt_allocate_memory(device, allocator, as_build_sizes_info.buildScratchSize, NULL,
			 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  
  geom_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  geom_info.scratchData.deviceAddress = as_scratch_buffer.device_address;
  geom_info.dstAccelerationStructure = as.as;
  
  const VkAccelerationStructureBuildRangeInfoKHR *as_build_range_info =
    &(VkAccelerationStructureBuildRangeInfoKHR){
    .primitiveCount = primitive_count,
  };

  VkCommandBuffer cmd = vkw_immediate_begin(device, immediate);
  vkCmdBuildAccelerationStructuresKHRp(cmd, 1, &geom_info,
				       &as_build_range_info);
  vkw_immediate_end(device, immediate, scratch_queue);

  VkAccelerationStructureDeviceAddressInfoKHR device_addr_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
    .accelerationStructure = as.as,
  };
  as.handle = vkGetAccelerationStructureDeviceAddressKHRp(device, &device_addr_info);

  vkrt_memory_free(allocator, as_scratch_buffer);

  return as;
}

vkrt_as vkrt_create_as2(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
			vkw_immediate_submit_buffer immediate, vkrt_as_level lvl,
			VkAccelerationStructureBuildGeometryInfoKHR geom_info,
			uint32_t build_range_info_count, uint32_t *primitive_counts,
			const VkAccelerationStructureBuildRangeInfoKHR **pp_build_range_infos) {
  vkrt_as as;
  VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
  };
  
  vkGetAccelerationStructureBuildSizesKHRp(device,
					   VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
					   &geom_info, primitive_counts,
					   &as_build_sizes_info); 
  as.memory =
    vkrt_allocate_memory(device, allocator,
			 as_build_sizes_info.accelerationStructureSize, NULL,
			 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
			 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

  VkAccelerationStructureCreateInfoKHR as_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
    .buffer = as.memory.buffer,
    .size = as_build_sizes_info.accelerationStructureSize,
    .type = lvl,
  };

  VK_CHECK(vkCreateAccelerationStructureKHRp(device, &as_info, NULL, &as.as));
  
  vkrt_memory as_scratch_buffer =
    vkrt_allocate_memory(device, allocator, as_build_sizes_info.buildScratchSize, NULL,
			 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  
  geom_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  geom_info.scratchData.deviceAddress = as_scratch_buffer.device_address;
  geom_info.dstAccelerationStructure = as.as;

  VkCommandBuffer cmd = vkw_immediate_begin(device, immediate);
  vkCmdBuildAccelerationStructuresKHRp(cmd, 1, &geom_info,
				       pp_build_range_infos);
  vkw_immediate_end(device, immediate, scratch_queue);

  VkAccelerationStructureDeviceAddressInfoKHR device_addr_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
    .accelerationStructure = as.as,
  };
  as.handle = vkGetAccelerationStructureDeviceAddressKHRp(device, &device_addr_info);

  vkrt_memory_free(allocator, as_scratch_buffer);

  return as;
}

void vkrt_destroy_as(VkDevice device, VmaAllocator allocator, vkrt_as as) {
  vkrt_memory_free(allocator, as.memory);
  vkDestroyAccelerationStructureKHRp(device, as.as, NULL);
}

typedef struct {
  uint64_t vertex_count;
  void *vertex_data;
  uint32_t sizeof_vertex;

  uint64_t index_count;
  void *index_data;
  uint32_t sizeof_index;

  uint32_t primitive_count; // number of triangles in the geometry data
  VkTransformMatrixKHR transform;
} vkrt_geom_data;

typedef struct {
  vkrt_memory vertex_buffer;
  vkrt_memory index_buffer;
  uint32_t vertex_count;
  uint32_t vertex_stride;
  uint32_t primitive_count;
  //
  vkrt_memory transform_buffer;
} vkrt_geom_data_gpu;
/*
vkrt_geom_data_gpu vkrt_load_gltf_mesh_gpu(VkDevice device, VmaAllocator allocator,
					   const char *fp, uint32_t mesh_idx,
					   VkTransformMatrixKHR transform) {
  cgltf_options options = {};
  cgltf_data *data = NULL;
  cgltf_result res = cgltf_parse_file(&options, fp, &data);
  if (res != cgltf_result_success) {
    fprintf(stderr, "Failed to load gltf file %s\n", fp);
    exit(1);
  }
  res = cgltf_load_buffers(&options, data, fp);
  if (res != cgltf_result_success) {
    fprintf(stderr, "Failed to load gltf file %s\n", fp);
    exit(1);
  }
  assert(mesh_idx < data->meshes_count && "Invalid mesh index");

  cgltf_mesh m = data->meshes[mesh_idx];

  size_t index_count = 0;
  size_t vertex_count = 0;
  // iterate through primitives to get sizes
  for (size_t i = 0; i < m.primitives_count; ++i) {
    cgltf_primitive p = m.primitives[i];
    index_count += cgltf_accessor_unpack_indices(p.indices, NULL, 1, 1);
    for (size_t j = 0; j < p.attributes_count; ++j) {
      if (p.attributes[j].type == cgltf_attribute_type_position) {
	vertex_count += p.attributes[j].data->count;
      }
    }
  }

  uint32_t *indices = malloc(sizeof(*indices) * index_count);
  vkrt_vertex_t *vertices = malloc(sizeof(*vertices) * vertex_count);
  assert(indices);
  assert(vertices);
  size_t vertex_offset = 0;
  size_t index_offset = 0;
    for (size_t i = 0; i < m.primitives_count; ++i) {
    cgltf_primitive p = m.primitives[i];
    size_t index_cnt = cgltf_accessor_unpack_indices(p.indices, NULL, 1, 1);
    cgltf_accessor_unpack_indices(p.indices, &indices[index_offset], 4, p.indices->count);
    index_offset += index_cnt;

    size_t vertex_cnt = 0;

    for (size_t j = 0; j < p.attributes_count; ++j) {
      if (p.attributes[j].type == cgltf_attribute_type_position) {
	cgltf_accessor *attr = p.attributes[j].data;
	float *p_vertices = (float *)attr->buffer_view->buffer->data +
	  attr->buffer_view->offset/sizeof(float) +
	  attr->offset/sizeof(float);
	size_t n = 0;
	vertex_cnt = attr->count;
	
	for(size_t k = 0; k < attr->count; ++k) {
	  vertices[vertex_offset + k].pos = (v3) {
	    p_vertices[n], p_vertices[n+1], p_vertices[n+2]
	  };
	  n += attr->stride/sizeof(float);
	}
      } else if (p.attributes[j].type == cgltf_attribute_type_normal) {
	cgltf_accessor *attr = p.attributes[j].data;
	float *p_normals = (float *)attr->buffer_view->buffer->data +
	  attr->buffer_view->offset/sizeof(float) +
	  attr->offset/sizeof(float);
	size_t n = 0;
	for(size_t k = 0; k < attr->count; ++k) {
	  vertices[vertex_offset + k].norm = (v3) {
	    p_normals[n], p_normals[n+1], p_normals[n+2]
	  };
	  n += attr->stride/sizeof(float);
	}
      } else if (p.attributes[j].type == cgltf_attribute_type_texcoord) {
	cgltf_accessor *attr = p.attributes[j].data;
	float *p_coords = (float *)attr->buffer_view->buffer->data +
	  attr->buffer_view->offset/sizeof(float) +
	  attr->offset/sizeof(float);
	size_t n = 0;
	for(size_t k = 0; k < attr->count; ++k) {
	  vertices[vertex_offset + k].u = p_coords[n];
	  vertices[vertex_offset + k].v = p_coords[n+1];
	  n += attr->stride/sizeof(float);
	}
      }
    }
    vertex_offset += vertex_cnt;
  }
  printf("Mesh loaded with: %lu indices (%lu primitives), %lu vertices\n",
	 index_count, index_count / 3, vertex_count);

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
  cgltf_free(data);

  vkrt_memory transform_buffer =
    vkrt_allocate_memory(device, allocator, sizeof(VkTransformMatrixKHR),
			 &transform, usage);

  return (vkrt_geom_data_gpu) {
    .vertex_buffer = vertex_buffer,
    .index_buffer = index_buffer,
    .vertex_count = vertex_count,
    .vertex_stride = sizeof(vkrt_vertex_t),
    .primitive_count = index_count / 3,
    .transform_buffer = transform_buffer,
  };
}

vkrt_geom_data_gpu *vkrt_load_gltf_all(VkDevice device, VmaAllocator allocator,
				       const char *fp, uint32_t *geom_count) {
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

  *geom_count = data->meshes_count;
  vkrt_geom_data_gpu *geom_data = calloc(sizeof(*geom_data), *geom_count);

  VkTransformMatrixKHR transform = {};  
  for(size_t i = 0; i < data->meshes_count; ++i) {
    HMM_Mat4 transform4 = {};
    cgltf_node_transform_world(&data->nodes[i], (float *)transform4.Elements);
    transform4 = HMM_TransposeM4(transform4);
    memcpy(&transform, &transform4.Elements, 12 * sizeof(float));
    
    geom_data[i] = vkrt_load_gltf_mesh_gpu(device, allocator, fp, i, transform);
  }
  cgltf_free(data);
  return geom_data;
}
*/
void vkrt_geom_data_gpu_free(VmaAllocator allocator, vkrt_geom_data_gpu data) {
  vkrt_memory_free(allocator, data.vertex_buffer);
  vkrt_memory_free(allocator, data.index_buffer);
  vkrt_memory_free(allocator, data.transform_buffer);
}

vkrt_as
vkrt_create_blas2(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
		  vkw_immediate_submit_buffer immediate, uint32_t geom_data_cnt,
		  vkrt_geom_data_gpu *geom_datas) {
  assert(geom_data_cnt >= 1 && "Must be at least 1 geometry");

  VkAccelerationStructureGeometryKHR *as_geom_infos =
    calloc(sizeof(*as_geom_infos), geom_data_cnt);

  VkAccelerationStructureBuildRangeInfoKHR *p_build_range_infos =
    calloc(sizeof(*p_build_range_infos), geom_data_cnt);

  const VkAccelerationStructureBuildRangeInfoKHR **pp_build_range_infos =
    calloc(sizeof(*pp_build_range_infos), geom_data_cnt);

  uint32_t *primitive_counts = calloc(sizeof(uint32_t), geom_data_cnt);

  for (uint32_t i = 0; i < geom_data_cnt; ++i) {
    as_geom_infos[i] = (VkAccelerationStructureGeometryKHR) {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
      .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
      .geometry.triangles.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
      .geometry.triangles.vertexData = geom_datas[i].vertex_buffer.device_address,
      .geometry.triangles.indexData = geom_datas[i].index_buffer.device_address,
      .geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
      .geometry.triangles.maxVertex = geom_datas[i].vertex_count,
      .geometry.triangles.vertexStride = geom_datas[i].vertex_stride,
      .geometry.triangles.indexType = VK_INDEX_TYPE_UINT32,
      .geometry.triangles.transformData = geom_datas[i].transform_buffer.device_address,
    };
    p_build_range_infos[i] = (VkAccelerationStructureBuildRangeInfoKHR) {
      .firstVertex = 0,
      .primitiveOffset = 0,
      .primitiveCount = geom_datas[i].primitive_count,
      .transformOffset = 0,
    };
    pp_build_range_infos[i] = &p_build_range_infos[i];
    primitive_counts[i] = geom_datas[i].primitive_count;
  }
  
  VkAccelerationStructureBuildGeometryInfoKHR as_build_geom_info =
    vkrt_as_build_geometry_info(vkrt_as_bottom, geom_data_cnt, as_geom_infos);

  vkrt_as blas = vkrt_create_as2(device, allocator, scratch_queue, immediate,
				 vkrt_as_bottom, as_build_geom_info, geom_data_cnt,
				 primitive_counts, pp_build_range_infos);

  free(pp_build_range_infos);
  free(p_build_range_infos);
  free(as_geom_infos);

  return blas;
}

vkrt_as
vkrt_create_blas3(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
		  vkw_immediate_submit_buffer immediate, vkrt_memory vertex_buffer,
		  vkrt_memory index_buffer, uint32_t primitive_count,
		  uint32_t vertex_count, vkrt_memory transform_buffer) {
  VkAccelerationStructureGeometryKHR as_geom_info = {};

  VkAccelerationStructureBuildRangeInfoKHR build_range_info = {};

  const VkAccelerationStructureBuildRangeInfoKHR *p_build_range_info =
    &build_range_info;
  

  as_geom_info = (VkAccelerationStructureGeometryKHR) {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
    .geometry.triangles.sType =
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
    .geometry.triangles.vertexData = vertex_buffer.device_address,
    .geometry.triangles.indexData = index_buffer.device_address,
    .geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
    .geometry.triangles.maxVertex = vertex_count,
    .geometry.triangles.vertexStride = sizeof(vkrt_vertex_t),
    .geometry.triangles.indexType = VK_INDEX_TYPE_UINT32,
    .geometry.triangles.transformData = transform_buffer.device_address,
  };
  build_range_info = (VkAccelerationStructureBuildRangeInfoKHR) {
    .firstVertex = 0,
    .primitiveOffset = 0,
    .primitiveCount = primitive_count,
    .transformOffset = 0,
  };
  
  VkAccelerationStructureBuildGeometryInfoKHR as_build_geom_info =
    vkrt_as_build_geometry_info(vkrt_as_bottom, 1, &as_geom_info);

  vkrt_as blas = vkrt_create_as2(device, allocator, scratch_queue, immediate,
				 vkrt_as_bottom, as_build_geom_info, 1,
				 &primitive_count, &p_build_range_info);

  return blas;
}

vkrt_as
vkrt_create_blas(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
		 vkw_immediate_submit_buffer immediate, uint32_t geom_data_cnt,
		 vkrt_geom_data *geom_datas, vkrt_memory *index_buffer,
		 vkrt_memory *vertex_buffer) {
  assert(geom_data_cnt >= 1 && "Must be at least 1 geometry");
  assert(geom_datas[0].sizeof_index == sizeof(uint32_t) &&
	 "Only supported index format is uint32_t (for now)");
  size_t sizeof_vertices = 0;
  size_t sizeof_indices = 0;
  for (uint32_t i = 0; i < geom_data_cnt; ++i) {
    for (size_t x = 0; x < geom_datas[i].index_count; ++x) {
      ((uint32_t *)geom_datas[i].index_data)[x] +=
	sizeof_vertices / geom_datas[i].sizeof_vertex;
    }
    sizeof_vertices += geom_datas[i].sizeof_vertex * geom_datas[i].vertex_count;
    sizeof_indices += geom_datas[i].sizeof_index * geom_datas[i].index_count;
  }

  uint8_t *vertex_memory = malloc(sizeof_vertices);
  uint8_t *index_memory = malloc(sizeof_indices);
  size_t highest_vertex = 0;
  size_t highest_index = 0;
  for (uint32_t i = 0; i < geom_data_cnt; ++i) {
    size_t vertex_copy = geom_datas[i].sizeof_vertex * geom_datas[i].vertex_count;
    size_t index_copy = geom_datas[i].sizeof_index * geom_datas[i].index_count;
    memcpy(vertex_memory + highest_vertex, geom_datas[i].vertex_data, vertex_copy);
    memcpy(index_memory + highest_index, geom_datas[i].index_data, index_copy);
    highest_vertex += vertex_copy;
    highest_index += index_copy;
  }

  VkBufferUsageFlagBits usage =
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  *vertex_buffer = vkrt_allocate_memory(device, allocator, sizeof_vertices,
						   vertex_memory, usage);
  *index_buffer = vkrt_allocate_memory(device, allocator, sizeof_indices,
						  index_memory, usage);

  VkTransformMatrixKHR *transform_memory =
    malloc(sizeof(VkTransformMatrixKHR) * geom_data_cnt);
  for (uint32_t i = 0; i < geom_data_cnt; ++i) {
    transform_memory[i] = geom_datas[i].transform;
  }
  
  vkrt_memory transform_buffer =
    vkrt_allocate_memory(device, allocator, sizeof(VkTransformMatrixKHR) * geom_data_cnt,
			 transform_memory, usage);
  
  VkAccelerationStructureGeometryKHR *as_geom_infos =
    calloc(sizeof(*as_geom_infos), geom_data_cnt);
  size_t vertex_offset = 0;
  size_t index_offset = 0;
  for (uint32_t i = 0; i < geom_data_cnt; ++i) {
    VkDeviceAddress transform_address =
      transform_buffer.device_address + i*sizeof(VkTransformMatrixKHR);
    
    as_geom_infos[i] = (VkAccelerationStructureGeometryKHR) {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
      .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
      .geometry.triangles.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
      .geometry.triangles.vertexData = vertex_buffer->device_address + vertex_offset,
      .geometry.triangles.indexData = index_buffer->device_address + index_offset,
      .geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
      .geometry.triangles.maxVertex = geom_datas[i].vertex_count,
      .geometry.triangles.vertexStride = geom_datas[i].sizeof_vertex,
      .geometry.triangles.indexType = VK_INDEX_TYPE_UINT32,
      .geometry.triangles.transformData = transform_address,
    };
    vertex_offset += geom_datas[i].sizeof_vertex * geom_datas[i].vertex_count;
    index_offset += geom_datas[i].sizeof_index * geom_datas[i].index_count;
  }
  
  VkAccelerationStructureBuildGeometryInfoKHR as_build_geom_info =
    vkrt_as_build_geometry_info(vkrt_as_bottom, geom_data_cnt, as_geom_infos);

  uint32_t primitive_count = 0;
  for (size_t i = 0; i < geom_data_cnt; ++i) {
    primitive_count += geom_datas[i].primitive_count;
  }
  
  vkrt_as blas = vkrt_create_as(device, allocator, scratch_queue, immediate,
				vkrt_as_bottom, as_build_geom_info,
				primitive_count);

  vkrt_memory_free(allocator, transform_buffer);
  free(as_geom_infos);
  free(vertex_memory);
  free(index_memory);
  
  return blas;
}

vkrt_as vkrt_create_tlas(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
			 vkw_immediate_submit_buffer immediate, uint64_t blas_cnt,
			 vkrt_as *blases, VkTransformMatrixKHR transform) {

  VkAccelerationStructureInstanceKHR *as_instances =
    calloc(sizeof(*as_instances), blas_cnt);
  for (uint64_t i = 0; i < blas_cnt; ++i) {
    as_instances[i] = (VkAccelerationStructureInstanceKHR) {
      .transform = transform,
      .instanceCustomIndex = 0,
      .mask = 0xFF,
      .instanceShaderBindingTableRecordOffset = 0,
      .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
      .accelerationStructureReference = blases[i].handle
    };
  }
  VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

  vkrt_memory instance_buffer =
    vkrt_allocate_memory(device, allocator, sizeof(*as_instances) * blas_cnt,
			 as_instances, usage);  
  free(as_instances);

  VkAccelerationStructureGeometryKHR as_geom_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
    .geometry.instances.sType =
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
    .geometry.instances.arrayOfPointers = VK_FALSE,
    .geometry.instances.data = instance_buffer.device_address,
  };
  
  VkAccelerationStructureBuildGeometryInfoKHR as_build_geom_info =
    vkrt_as_build_geometry_info(vkrt_as_top, 1, &as_geom_info);
  
  vkrt_as tlas = vkrt_create_as(device, allocator, scratch_queue, immediate,
				vkrt_as_top, as_build_geom_info,
				blas_cnt);

  vkrt_memory_free(allocator, instance_buffer);
  return tlas;
}

typedef struct {
  uint32_t ds_count;
  VkWriteDescriptorSet *write_ds;
  VkDescriptorSet dst_set;
  void *p_next; // ds_count pNext pointers to make sure they are valid
  VkDescriptorBufferInfo *buffer_infos; // make sure pointers are valid
  VkDescriptorImageInfo *image_infos;
  size_t image_info_idx;
  size_t buffer_info_idx;
} vkrt_ds_writer;

vkrt_ds_writer vkrt_ds_writer_create(uint32_t ds_count, VkDescriptorSet dst_set) {
  VkWriteDescriptorSet *ds = calloc(sizeof(*ds), ds_count);
  // all (valid) pNext values are the same size, so we allocate based on that fact
  void *pnext = calloc(sizeof(VkWriteDescriptorSetAccelerationStructureKHR), ds_count);
  
  return (vkrt_ds_writer) {
    ds_count,
    ds,
    dst_set,
    pnext,
    calloc(sizeof(VkDescriptorBufferInfo), ds_count),
    calloc(sizeof(VkDescriptorImageInfo), ds_count),
  };
}

void vkrt_ds_writer_add_as(vkrt_ds_writer *writer, uint32_t binding,
			   VkAccelerationStructureKHR *as) {
  ((VkWriteDescriptorSetAccelerationStructureKHR*)writer->p_next)[binding] =
    (VkWriteDescriptorSetAccelerationStructureKHR) {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
    .accelerationStructureCount = 1,
    .pAccelerationStructures = as,
  };
  writer->write_ds[binding] = (VkWriteDescriptorSet) {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = &((VkWriteDescriptorSetAccelerationStructureKHR*)writer->p_next)[binding],
    .dstSet = writer->dst_set,
    .dstBinding = binding,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
  };
}

void vkrt_ds_writer_add_image(vkrt_ds_writer *writer, uint32_t binding,
			       VkImageView view) {
  writer->image_infos[writer->image_info_idx++] = (VkDescriptorImageInfo) {
    .sampler = VK_NULL_HANDLE,
    .imageView = view,
    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
  };
  writer->write_ds[binding] = (VkWriteDescriptorSet) {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pImageInfo = &writer->image_infos[writer->image_info_idx - 1],
    .dstSet = writer->dst_set,
    .dstBinding = binding,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
  };
}

void vkrt_ds_writer_add_sampled_images(vkrt_ds_writer *writer, uint32_t binding,
				       uint32_t image_count, vkw_image *images) {
  size_t start = writer->image_info_idx;
  for (size_t i = 0; i < image_count; ++i) {
    writer->image_infos[writer->image_info_idx++] = (VkDescriptorImageInfo) {
      .sampler = images[i].sampler,
      .imageView = images[i].view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
  }

  writer->write_ds[binding] = (VkWriteDescriptorSet) {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pImageInfo = &writer->image_infos[start],
    .dstSet = writer->dst_set,
    .dstBinding = binding,
    .descriptorCount = image_count,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
  };
}

void vkrt_ds_writer_add_buffer(vkrt_ds_writer *writer, uint32_t binding,
				VkBuffer buffer, uint64_t offset, uint64_t range) {
  writer->buffer_infos[writer->buffer_info_idx++] = (VkDescriptorBufferInfo) {
    .buffer = buffer,
    .offset = offset,
    .range = range
  };
  writer->write_ds[binding] = (VkWriteDescriptorSet) {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pBufferInfo = &writer->buffer_infos[writer->buffer_info_idx - 1],
    .dstSet = writer->dst_set,
    .dstBinding = binding,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
  };
}

void vkrt_ds_writer_write(VkDevice device, vkrt_ds_writer writer) {
  vkUpdateDescriptorSets(device, writer.ds_count, writer.write_ds, 0, NULL);
}

void vkrt_ds_writer_free(vkrt_ds_writer *writer) {
  writer->ds_count = 0;
  free(writer->write_ds);
  free(writer->p_next);
  free(writer->image_infos);
  free(writer->buffer_infos);
}
#endif // VK_RT_HELP_H_
