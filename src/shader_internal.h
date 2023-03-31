#ifndef VKENV_SHADER_INTERNAL_H
#define VKENV_SHADER_INTERNAL_H

#include <volk.h>

#include "shader.h"
#include "descriptor_structs.h"

#include <vector>

void processDescriptorSet(descriptor::Set set, size_t frameCount, VkDevice device);
			  

#endif
