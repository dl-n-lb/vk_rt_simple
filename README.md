# Simple Vulkan Ray Tracing Code
This was put together to get used to the vulkan/vulkan ray tracing apis. The raytracing itself is literally the most basic possible integrator where it just randomly samples on a cosine distribution.
My code depends on:
- [cgltf](https://github.com/jkuhlmann/cgltf)
- [vulkan memory allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [handmade math](https://github.com/HandmadeMath/HandmadeMath)
- [stb_image.h](https://github.com/nothings/stb)
- [dear imgui](https://github.com/ocornut/imgui)
- [cimgui](https://github.com/cimgui/cimgui)

To compile and run:
```zsh
> make main
> ./main
```
Expect to see a more tidy/practical implementation on my github soon, possibly with more features implemented.

(MIT license - but please don't actually use this.)
