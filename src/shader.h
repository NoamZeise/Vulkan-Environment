#ifndef VK_ENV_SHADER_H
#define VK_ENV_SHADER_H

#include <volk.h>

#include "descriptor_structs.h"

// TO FIX FROM OLD
// - desciptor set keeps track of if dynamic, for pipeline binding
// - binding needed reference to parent set, for PrepareShaderbuffersets fn
// - binding needed setCount (number of duplicates for a set (ie per frames))
// for Prepareshaderbuffersets fn
// - binding needed viewsPerSet boolean, ie. perFrameImageViews(ie sampling colour attachment in final) vs image views(ie texture views - doesnt change per frame) for Prepareshaderbuffersets.



struct DescriptorBinding {
    VkDescriptorType descriptorType;
    size_t bindingIndex;

    //only used by storage/uniform descriptor types 
    size_t storedTypeSize;
    size_t storedArraySize = 1;
    size_t dynamicBufferCount = 1;
    size_t descriptorCount = 1;

    size_t memoryOffset;
    VkDeviceSize slotSize;
    void* pHostVisibleMemory;

    //only used by sampler and view descriptors types -can this be better abstracted???
    VkImageView *imageViews;
    VkSampler *samplers;
};

struct DescriptSet {
    VkDescriptorSetLayout layout;
    
    VkDescriptorSet* sets;
    size_t setCount;
    
    DescriptorBinding* descriptorBindings;
    size_t descriptorBindingCount;

    DescriptorBinding* bindings;
    size_t bindingsCount;
};

#endif
