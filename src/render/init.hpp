#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

VkInstance createInstance();

void destroyInstance(VkInstance instance);
