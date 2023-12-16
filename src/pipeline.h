#ifndef PIPELINE_H
#define PIPELINE_H

#include <volk.h>
#include <GLFW/glfw3.h>

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>

#include <stdint.h>
#include <vector>
#include "shader_internal.h"
#include "logger.h"

class Pipeline
{
public:
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout layout;
  VkPipeline pipeline;
  std::vector<DS::DescriptorSet*> descriptorSets;
  std::vector<bool> descriptorSetsActive;

  void setDescSetState(DS::DescriptorSet* set, bool isActive) {
      for(int i = 0; i < descriptorSets.size(); i++) {
	  if(descriptorSets[i] == set) {
	      descriptorSetsActive[i] = isActive;
	  }
      }
  }
    
  void begin(VkCommandBuffer cmdBuff, size_t frameIndex) {
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

  void bindDynamicDS(VkCommandBuffer cmdBuff, DS::DescriptorSet *ds, size_t frameIndex,
		     uint32_t dynOffset) {
      for (size_t i = 0; i < descriptorSets.size(); i++)
	  if(descriptorSets[i]->dynamicBuffer)
	      if(descriptorSets[i] == ds)
		  vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
					  static_cast<uint32_t>(i), 1,
					  &descriptorSets[i]->sets[frameIndex],
					  1, &dynOffset);
  }
    
  void destroy(VkDevice device) {
      vkDestroyPipeline(device, pipeline, nullptr);
      vkDestroyPipelineLayout(device, layout, nullptr);
  }
};


#endif
