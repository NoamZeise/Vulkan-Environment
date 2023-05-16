#ifndef PARTS_DESCRIPTORS_H
#define PARTS_DESCRIPTORS_H

#include <volk.h>
#include "../shader_internal.h"
#include <vector>

struct DeviceState;

namespace part {
    namespace create {
        void DescriptorSetLayout(
		VkDevice device,
		DS::DescriptorSet *ds,
		std::vector<DS::Binding*> bindings,
		VkShaderStageFlagBits stageFlags);
        void DescriptorPoolAndSet(
		VkDevice device,
		VkDescriptorPool* pool,
		std::vector<DS::DescriptorSet*> descriptorSets,
		uint32_t frameCount);
        void PrepareShaderBufferSets(
		DeviceState base,
		std::vector<DS::Binding*> ds,
		VkBuffer* buffer,
		VkDeviceMemory* memory);
    }
}

#endif
