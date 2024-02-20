#ifndef VK_INIT_H_
#define VK_INIT_H_

#include "vulkan/vulkan.h"

#include <stdlib.h>
#include <stdbool.h>

#ifndef VKI_CALLOC
#define VKI_CALLOC calloc
#endif

#ifndef VKI_FREE
#define VKI_FREE free
#endif

#ifndef VKI_REALLOC
#define VKI_REALLOC realloc
#endif

const char* string_VkResult(VkResult input_value);

#define VK_CHECK(x)							\
  do {									\
    VkResult err = x;							\
    if (err) {								\
      fprintf(stderr, "Vulkan Error: %s", string_VkResult(err));	\
      exit(1);								\
    }									\
  } while(0);

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance,
					const VkDebugUtilsMessengerCreateInfoEXT
					*create_info,
					const VkAllocationCallbacks *alloc,
					VkDebugUtilsMessengerEXT *out_messenger);

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
				     VkDebugUtilsMessengerEXT messenger,
				     const VkAllocationCallbacks *alloc);

typedef struct {
  char **data;
  size_t len;
  size_t cap;
} vki_darr_str_t;

vki_darr_str_t vki_darr_str_init(size_t initial_cap);

void vki_darr_str_free(vki_darr_str_t *arr);

// we dont own the string!
void vki_darr_str_push(vki_darr_str_t *arr, char *new);

typedef struct {
  VkApplicationInfo app_info;
  vki_darr_str_t extension_names;
  vki_darr_str_t layer_names;

  bool enable_debug_messenger;
  VkDebugUtilsMessengerCreateInfoEXT dbg_create_info;
} vki_instance_builder;

typedef struct {
  VkInstance instance;
  VkDebugUtilsMessengerEXT messenger;
} vki_instance;

VkApplicationInfo vki_default_app_info();

vki_instance_builder vki_new_instance_builder();

void vki_set_api_version(vki_instance_builder *b, uint32_t api_version);

VKAPI_ATTR VkBool32 VKAPI_CALL
vki_default_debug_messenger_cb(VkDebugUtilsMessageSeverityFlagBitsEXT msg_sev,
			       VkDebugUtilsMessageTypeFlagsEXT msg_type,
			       const VkDebugUtilsMessengerCallbackDataEXT *d,
			       void *user_data);

void vki_enable_debug_messenger(vki_instance_builder *b,
				VkDebugUtilsMessengerCreateInfoEXT *info);

void vki_enable_layer(vki_instance_builder *b, char *name);

void vki_enable_layers(vki_instance_builder *b, size_t count,
		       char *names[restrict count]);

void vki_enable_validation(vki_instance_builder *b);

void vki_enable_extension(vki_instance_builder *b, char *name);

void vki_enable_extensions(vki_instance_builder *b, size_t count,
			   char *names[restrict count]);

vki_instance vki_instance_build(vki_instance_builder b);

typedef struct {
  VkInstance instance;
  VkSurfaceKHR surface;
  
  uint32_t min_version;
  VkPhysicalDeviceFeatures2 features;
  VkPhysicalDeviceVulkan11Features features11;
  VkPhysicalDeviceVulkan12Features features12;
  VkPhysicalDeviceVulkan13Features features13;
  
  vki_darr_str_t extensions;
  void *pnext;

  bool selected;
  VkPhysicalDevice physical_device;  
} vki_physical_device;

typedef struct {
  VkDevice device;
  VkPhysicalDevice physical_device;
  
  VkQueueFamilyProperties *queue_families;
  uint32_t queue_family_count;
} vki_device;

vki_physical_device vki_physical_device_init(VkInstance instance,
					     uint32_t vulkan_version);

void vki_enable_device_extension(vki_physical_device *s, char *name);

void vki_set_surface(vki_physical_device *s, VkSurfaceKHR surface);

void vki_set_features13(vki_physical_device *s,
			VkPhysicalDeviceVulkan13Features features);

void vki_set_features12(vki_physical_device *s,
			VkPhysicalDeviceVulkan12Features features);

void vki_set_features11(vki_physical_device *s,
			VkPhysicalDeviceVulkan11Features features);

void vki_set_features(vki_physical_device *s,
		      VkPhysicalDeviceFeatures2 features);

void vki_enable_features_pnext(vki_physical_device *s, void *pnext);

void vki_physical_device_select(vki_physical_device *s);

vki_device vki_device_create(vki_physical_device pdev);

VkQueue vki_device_get_queue(vki_device dev, uint32_t type, uint32_t *family);

void vki_device_cleanup(vki_device dev);

typedef struct {
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkSurfaceKHR surface;
  uint32_t desired_width, desired_height;

  VkSurfaceFormatKHR desired_format;
  uint32_t desired_present_mode;

  uint32_t image_usage_flags;
} vki_swapchain_builder;

typedef struct {
  VkSwapchainKHR swapchain;
  VkImage *images;
  VkImageView *image_views;
  uint32_t image_count;
  VkExtent2D extent;
} vki_swapchain;

void vki_set_desired_format(vki_swapchain_builder *b, VkSurfaceFormatKHR f);

void vki_set_desired_present_mode(vki_swapchain_builder *b, uint32_t mode);

void vki_add_image_usage_flags(vki_swapchain_builder *b, uint32_t flags);

vki_swapchain vki_swapchain_build(vki_swapchain_builder b);

void vki_destroy_swapchain(VkDevice device, vki_swapchain swapchain);

#endif
#ifdef VK_INIT_IMPL
// function below copied from this file
//#include <vulkan/vk_enum_string_helper.h>
// since the file itself is not c compatible (would have to be compiled with c++)
// since it includes <string> 
const char* string_VkResult(VkResult input_value)
{
    switch (input_value)
    {
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
            return "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
#ifdef VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
#endif // VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_ERROR_NOT_PERMITTED_KHR:
            return "VK_ERROR_NOT_PERMITTED_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        default:
            return "Unhandled VkResult";
    }
}

#define VK_CHECK(x)							\
  do {									\
    VkResult err = x;							\
    if (err) {								\
      fprintf(stderr, "Vulkan Error: %s", string_VkResult(err));	\
      exit(1);								\
    }									\
  } while(0);

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance,
					const VkDebugUtilsMessengerCreateInfoEXT
					*create_info,
					const VkAllocationCallbacks *alloc,
					VkDebugUtilsMessengerEXT *out_messenger)
{
  PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (fn) {
    return fn(instance, create_info, alloc, out_messenger);
  }
  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
				     VkDebugUtilsMessengerEXT messenger,
				     const VkAllocationCallbacks *alloc) {
  PFN_vkDestroyDebugUtilsMessengerEXT fn = (PFN_vkDestroyDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (fn) {
    fn(instance, messenger, alloc);
  }
}

vki_darr_str_t vki_darr_str_init(size_t initial_cap) {
  return (vki_darr_str_t) {
    .data = VKI_CALLOC(initial_cap, sizeof(char *)),
    .len = 0,
    .cap = initial_cap,
  };
}

void vki_darr_str_free(vki_darr_str_t *arr) {
  VKI_FREE(arr->data);
  *arr = (vki_darr_str_t){};
}

// we dont own the string!
void vki_darr_str_push(vki_darr_str_t *arr, char *new) {
  if (arr->len + 1 > arr->cap) {
    if (arr->cap == 0) { arr->cap = 1; }
    arr->cap *= 2;
    arr->data = VKI_REALLOC(arr->data, arr->cap * sizeof(char *));
  }
  arr->data[arr->len++] = new;
}

VkApplicationInfo vki_default_app_info() {
  return (VkApplicationInfo) {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Vulkan App",
    .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
    .pEngineName = "",
    .engineVersion = VK_MAKE_VERSION(0, 0, 0),
    .apiVersion = VK_API_VERSION_1_3,
  };
}

vki_instance_builder vki_new_instance_builder() {
  return (vki_instance_builder) {
    .app_info = vki_default_app_info(),
    .layer_names = vki_darr_str_init(1),
    .extension_names = vki_darr_str_init(1),
  };
}

void vki_set_api_version(vki_instance_builder *b, uint32_t api_version) {
  b->app_info.apiVersion = api_version;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
vki_default_debug_messenger_cb(VkDebugUtilsMessageSeverityFlagBitsEXT msg_sev,
			       VkDebugUtilsMessageTypeFlagsEXT msg_type,
			       const VkDebugUtilsMessengerCallbackDataEXT *d,
			       void *user_data) {
  (void) msg_sev;
  (void) msg_type;
  (void) user_data;
  fprintf(stderr, "Validation Layer: %s\n", d->pMessage);

  return VK_FALSE;
}

void vki_enable_debug_messenger(vki_instance_builder *b,
				VkDebugUtilsMessengerCreateInfoEXT *info) {
  b->enable_debug_messenger = true;
  if (info != NULL) {
    info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    b->dbg_create_info = *info;
    return;
  }  
  b->dbg_create_info = (VkDebugUtilsMessengerCreateInfoEXT) {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = vki_default_debug_messenger_cb,
  };
}

void vki_enable_layer(vki_instance_builder *b, char *name) {
  vki_darr_str_push(&b->layer_names, name);
}

void vki_enable_layers(vki_instance_builder *b, size_t count, char *names[restrict count]) {
  for (size_t i = 0; i < count; ++i) {
    vki_darr_str_push(&b->layer_names, names[i]);
  }
}

void vki_enable_validation(vki_instance_builder *b) {
  vki_enable_layer(b, "VK_LAYER_KHRONOS_validation");
}

void vki_enable_extension(vki_instance_builder *b, char *name) {
  vki_darr_str_push(&b->extension_names, name);
}

void vki_enable_extensions(vki_instance_builder *b, size_t count, char *names[restrict count]) {
  for (size_t i = 0; i < count; ++i) {
    vki_darr_str_push(&b->extension_names, names[i]);
  }
}

vki_instance vki_instance_build(vki_instance_builder b) {
  VkInstanceCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &b.app_info,
    .enabledExtensionCount = b.extension_names.len,
    .ppEnabledExtensionNames = (const char * const *)b.extension_names.data,
    .enabledLayerCount = b.layer_names.len,
    .ppEnabledLayerNames = (const char * const *)b.layer_names.data,    
  };
  vki_instance res;
  VK_CHECK(vkCreateInstance(&info, NULL, &res.instance));
  if (b.enable_debug_messenger) {
    VK_CHECK(vkCreateDebugUtilsMessengerEXT(res.instance, &b.dbg_create_info,
					    NULL, &res.messenger));
  }
  vki_darr_str_free(&b.layer_names);
  vki_darr_str_free(&b.extension_names);
  return res;
}

vki_physical_device vki_physical_device_init(VkInstance instance,
					     uint32_t vulkan_version) {
  return (vki_physical_device) {
    .instance = instance,
    .min_version = vulkan_version,
  };
}

void vki_enable_device_extension(vki_physical_device *s, char *name) {
  vki_darr_str_push(&s->extensions, name);
}

void vki_set_surface(vki_physical_device *s, VkSurfaceKHR surface) {
  s->surface = surface;
  // if we want to draw to a surface, we need a swapchain too
  vki_enable_device_extension(s, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void vki_set_features13(vki_physical_device *s,
			VkPhysicalDeviceVulkan13Features features) {
  s->features13 = features;
}

void vki_set_features12(vki_physical_device *s,
			VkPhysicalDeviceVulkan12Features features) {
  s->features12 = features;
}

void vki_set_features11(vki_physical_device *s,
			VkPhysicalDeviceVulkan11Features features) {
  s->features11 = features;
}

void vki_set_features(vki_physical_device *s,
			VkPhysicalDeviceFeatures2 features) {
  s->features = features;
}

void vki_enable_features_pnext(vki_physical_device *s, void *pnext) {
  s->pnext = pnext;
}

void vki_physical_device_select(vki_physical_device *s) {
  // TODO: any real selection process
  // right now just grab the first device that matches the api version :3
  uint32_t dev_cnt;
  vkEnumeratePhysicalDevices(s->instance, &dev_cnt, NULL);
  VkPhysicalDevice *devs = VKI_CALLOC(dev_cnt, sizeof(VkPhysicalDevice));
  vkEnumeratePhysicalDevices(s->instance, &dev_cnt, devs);

  for (uint32_t i = 0; i < dev_cnt; ++i) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(devs[i], &props);
    if (props.apiVersion >= s->min_version) {
      s->physical_device = devs[i];
      break;
    }
  }
  VKI_FREE(devs);
  if (s->physical_device != VK_NULL_HANDLE) {
    s->selected = true;
  } else {
    fprintf(stderr, "Failed to find a physical device that works\n");
    exit(1);
  }
}

vki_device vki_device_create(vki_physical_device pdev) {
  pdev.features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  pdev.features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  pdev.features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
  pdev.features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

  pdev.features.pNext = &pdev.features11;
  pdev.features11.pNext = &pdev.features12;
  pdev.features12.pNext = &pdev.features13;
  pdev.features13.pNext = pdev.pnext;

  // create queues for every queue family
  uint32_t qf_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(pdev.physical_device,
					   &qf_count, NULL);
  VkQueueFamilyProperties *qf_props = VKI_CALLOC(qf_count, sizeof(*qf_props));
  vkGetPhysicalDeviceQueueFamilyProperties(pdev.physical_device,
					   &qf_count, qf_props);
  VkDeviceQueueCreateInfo *queue_infos = VKI_CALLOC(qf_count,
						    sizeof(*queue_infos));
  
  float queue_priorities[32];
  for(size_t i = 0; i < 32; ++i) {
    queue_priorities[i] = 1.f;
  }
  for (uint32_t i = 0; i < qf_count; ++i) {
    queue_infos[i] = (VkDeviceQueueCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = i,
      .queueCount = qf_props[i].queueCount,
      .pQueuePriorities = queue_priorities,
    };
  }
  
  VkDeviceCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &pdev.features,
    .enabledExtensionCount = pdev.extensions.len,
    .ppEnabledExtensionNames = (const char* const*)pdev.extensions.data,
    .queueCreateInfoCount = qf_count,
    .pQueueCreateInfos = queue_infos,
  };

  VkDevice device;

  VK_CHECK(vkCreateDevice(pdev.physical_device, &info, NULL, &device));
  
  VKI_FREE(queue_infos);
  vki_darr_str_free(&pdev.extensions);
  return (vki_device) {
    .device = device,
    .physical_device = pdev.physical_device,
    .queue_families = qf_props,
    .queue_family_count = qf_count,
  };
}

VkQueue vki_device_get_queue(vki_device dev, uint32_t type, uint32_t *family) {
  *family = dev.queue_family_count;
  for (uint32_t i = 0; i < dev.queue_family_count; ++i) {
    if (dev.queue_families[i].queueFlags & type) {
      *family = i;
      break;
    }
  }
  if (*family >= dev.queue_family_count) {
    // error
    return VK_NULL_HANDLE;
  }
  VkQueue res;
  vkGetDeviceQueue(dev.device, *family, 0, &res);
  return res;
}

void vki_device_cleanup(vki_device dev) {
  VKI_FREE(dev.queue_families);
}

void vki_set_desired_format(vki_swapchain_builder *b, VkSurfaceFormatKHR f) {
  b->desired_format = f;
}

void vki_set_desired_present_mode(vki_swapchain_builder *b, uint32_t mode) {
  b->desired_present_mode = mode;
}

void vki_add_image_usage_flags(vki_swapchain_builder *b, uint32_t flags) {
  b->image_usage_flags |= flags;
}

vki_swapchain vki_swapchain_build(vki_swapchain_builder b) {
  VkSurfaceCapabilitiesKHR abilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(b.physical_device,
					    b.surface, &abilities);
  // clamp dimensions
  if (b.desired_width < abilities.minImageExtent.width) {
    /* fprintf(stderr, "Width adjusted from %d to %d due to device constraints\n", */
    /* 	    b.desired_width, */
    /* 	    abilities.minImageExtent.width);	     */
    b.desired_width = abilities.minImageExtent.width;
  }
  if (b.desired_width > abilities.maxImageExtent.width) {
    /* fprintf(stderr, "Width adjusted from %d to %d due to device constraints\n", */
    /* 	    b.desired_width, */
    /* 	    abilities.maxImageExtent.width); */
    b.desired_width = abilities.maxImageExtent.width;
  }
  if (b.desired_height < abilities.minImageExtent.height) {
    /* fprintf(stderr, "Height adjusted from %d to %d due to device constraints\n", */
    /* 	    b.desired_height, */
    /* 	    abilities.minImageExtent.height); */
    b.desired_height = abilities.minImageExtent.height;
  }
  if (b.desired_height > abilities.maxImageExtent.height) {
    /* fprintf(stderr, "Height adjusted from %d to %d due to device constraints\n", */
    /* 	    b.desired_height, */
    /* 	    abilities.maxImageExtent.height); */
    b.desired_height = abilities.maxImageExtent.height;
  }

  uint32_t pm_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(b.physical_device, b.surface,
					    &pm_count, NULL);
  if (pm_count == 0) {
    fprintf(stderr, "Device does not support presenting to this surface!");
    exit(1);
  }
  VkPresentModeKHR *modes = VKI_CALLOC(pm_count, sizeof(*modes));
  vkGetPhysicalDeviceSurfacePresentModesKHR(b.physical_device, b.surface,
					    &pm_count, modes);
  bool mode_valid = false;
  for (uint32_t i = 0; i < pm_count; ++i) {
    if (modes[i] == b.desired_present_mode) {
      mode_valid = true;
      break;
    }
  }
  VKI_FREE(modes);
  if (!mode_valid) {
    fprintf(stderr, "Device does not support desired present mode");
    exit(1);
  }

  VkSwapchainCreateInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = b.surface,
    .minImageCount = abilities.minImageCount,
    .imageFormat = b.desired_format.format,
    .imageColorSpace = b.desired_format.colorSpace,
    .imageExtent = (VkExtent2D) {b.desired_width, b.desired_height},
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | b.image_usage_flags,
    .preTransform = abilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = b.desired_present_mode,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  vki_swapchain res;
  VK_CHECK(vkCreateSwapchainKHR(b.device, &info, NULL, &res.swapchain));

  vkGetSwapchainImagesKHR(b.device, res.swapchain, &res.image_count, NULL);
  res.images = VKI_CALLOC(res.image_count, sizeof(*res.images));
  vkGetSwapchainImagesKHR(b.device, res.swapchain,
			  &res.image_count, res.images);
  res.image_views = VKI_CALLOC(res.image_count, sizeof(*res.image_views));
  for (uint32_t i = 0; i < res.image_count; ++i) {
    VkImageViewCreateInfo image_view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = res.images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = b.desired_format.format,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.levelCount = 1,
      .subresourceRange.layerCount = 1,
    };
    VK_CHECK(vkCreateImageView(b.device, &image_view_info, NULL,
			       &res.image_views[i]));
  }
  res.extent = (VkExtent2D) {b.desired_width, b.desired_height};
  return res;
}

void vki_destroy_swapchain(VkDevice device, vki_swapchain swapchain) {
  vkDestroySwapchainKHR(device, swapchain.swapchain, NULL);
  for (uint32_t i = 0; i < swapchain.image_count; ++i) {
    vkDestroyImageView(device, swapchain.image_views[i], NULL);
  }
  free(swapchain.images);
  free(swapchain.image_views);
}
#endif
