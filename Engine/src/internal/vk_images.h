#pragma once 
#include <vulkan/vulkan.h>


namespace Engine
{
	namespace vkUtil
	{
		void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
		void copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
		void generateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);
	}
}
