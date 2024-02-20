#ifndef VK_HELP_H_
#define VK_HELP_H_
#include "vulkan/vulkan.h"
#include <stdlib.h>
#include <stdio.h>
VkImageCreateInfo vkh_image_create_info(VkFormat format, VkExtent3D extent,
					VkImageUsageFlags usage);

VkImageViewCreateInfo vkh_image_view_create_info(VkFormat format,
						 VkImage image,
						 VkImageAspectFlags flags);

bool vkh_load_shader_module(const char *path, VkDevice device,
			    VkShaderModule *out_module);

VkImageSubresourceRange
vkh_image_subresource_range(VkImageAspectFlags flags);

void vkh_transition_image(VkCommandBuffer cmd, VkImage img,
			  VkImageLayout now, VkImageLayout next);

void vkh_copy_image_to_image(VkCommandBuffer cmd, VkImage src,
			     VkImage dst, VkExtent2D srce,
			     VkExtent2D dste);

#endif
#ifdef VK_HELP_IMPL
VkImageCreateInfo vkh_image_create_info(VkFormat format, VkExtent3D extent,
					VkImageUsageFlags usage) {
  return (VkImageCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = format,
    .extent = extent,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = usage
  };
}

VkImageViewCreateInfo vkh_image_view_create_info(VkFormat format,
						 VkImage image,
						 VkImageAspectFlags flags) {
  return (VkImageViewCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .image = image,
    .format = format,
    .subresourceRange.baseMipLevel = 0,
    .subresourceRange.levelCount = 1,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount = 1,
    .subresourceRange.aspectMask = flags
  };
}

bool vkh_load_shader_module(const char *path, VkDevice device,
			    VkShaderModule *out_module) {
  FILE *file = fopen(path, "rb");
  if (!file) { return false; }

  fseek(file, 0, SEEK_END);
  uint32_t file_size = (uint32_t)ftell(file);
  fseek(file, 0, SEEK_SET);
  char *buffer = malloc(file_size);
  fread(buffer, file_size, 1, file);
  fclose(file);

  VkShaderModuleCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = file_size,
    .pCode = (uint32_t *)buffer,
  };

  if (vkCreateShaderModule(device, &info, NULL, out_module)
      != VK_SUCCESS) {
    return false;
  }
  return true;
}

VkImageSubresourceRange
vkh_image_subresource_range(VkImageAspectFlags flags) {
  return (VkImageSubresourceRange) {
    .aspectMask = flags,
    .levelCount = VK_REMAINING_MIP_LEVELS,
    .layerCount = VK_REMAINING_ARRAY_LAYERS,
  };
}

void vkh_transition_image(VkCommandBuffer cmd, VkImage img,
			  VkImageLayout now, VkImageLayout next) {
  VkImageMemoryBarrier2 barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
 .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT |
                     VK_ACCESS_2_MEMORY_READ_BIT,
    
    .oldLayout = now,
    .newLayout = next
  };

  VkImageAspectFlags flags =
    (next == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ?
    VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

  VkImageSubresourceRange rng = vkh_image_subresource_range(flags);
  barrier.subresourceRange = rng;
  barrier.image = img;

  VkDependencyInfo dep_info = {
    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &barrier
  };

  vkCmdPipelineBarrier2(cmd, &dep_info);
}

void vkh_copy_image_to_image(VkCommandBuffer cmd, VkImage src,
			     VkImage dst, VkExtent2D srce,
			     VkExtent2D dste) {
  VkImageBlit2 blit_region = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
  
    .srcOffsets[1].x = srce.width,
    .srcOffsets[1].y = srce.height,
    .srcOffsets[1].z = 1,

    .dstOffsets[1].x = dste.width,
    .dstOffsets[1].y = dste.height,
    .dstOffsets[1].z = 1,

    .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .srcSubresource.layerCount = 1,  
    .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .dstSubresource.layerCount = 1,
  };

  VkBlitImageInfo2 blit_info = {
    .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
    .dstImage = dst,
    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .srcImage = src,
    .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    .filter = VK_FILTER_LINEAR,
    .regionCount = 1,
    .pRegions = &blit_region,
  };

  vkCmdBlitImage2(cmd, &blit_info);
}
#endif
