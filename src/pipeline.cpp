#include "pipeline.h"


Pipeline::Pipeline(
	VkPipelineLayout layout, VkPipeline pipeline, std::vector<DS::DescriptorSet*> sets) {
    this->descriptorSets = sets;
    this->layout = layout;
    this->pipeline = pipeline;
    this->descriptorSetsActive = std::vector<bool>(descriptorSets.size(), true);
}

void Pipeline::setDescSetState(DS::DescriptorSet *set, bool isActive) {
    for(int i = 0; i < descriptorSets.size(); i++) {
	if(descriptorSets[i] == set) {
	    descriptorSetsActive[i] = isActive;
	}
    }
}

void Pipeline::begin(VkCommandBuffer cmdBuff, size_t frameIndex) {
    //bind non dynamic descriptor sets
    int bindOffset = 0;
    for (size_t i = 0; i < descriptorSets.size(); i++) {
	if(descriptorSetsActive[i] &&
	   !descriptorSets[i]->dynamicBuffer &&
	   descriptorSets[i]->sets.size() != 0)
	    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
				    static_cast<uint32_t>(i - bindOffset), 1,
				    &descriptorSets[i]->sets[frameIndex],
				    0, nullptr);
	else if(!descriptorSetsActive[i]) {
	    LOG("inactive slot: " << i);
	    bindOffset++;
	}
    }
    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void Pipeline::bindDynamicDS(
	VkCommandBuffer cmdBuff, DS::DescriptorSet *ds, size_t frameIndex, uint32_t dynOffset) {
    for (size_t i = 0; i < descriptorSets.size(); i++)
	if(descriptorSets[i]->dynamicBuffer)
	    if(descriptorSets[i] == ds)
		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
					static_cast<uint32_t>(i), 1,
					&descriptorSets[i]->sets[frameIndex],
					1, &dynOffset);
}

void Pipeline::destroy(VkDevice device) {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, layout, nullptr);
}
