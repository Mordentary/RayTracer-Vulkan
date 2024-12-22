#include "vulkan_command_list.hpp"
#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_fence.hpp"
#include "vulkan_texture.hpp"
#include "vulkan_descriptor.hpp"
#include "vulkan_pipeline.hpp"
//#include "vulkan_descriptor.hpp"
//#include "vulkan_descriptor_allocator.hpp"
//#include "vulkan_constant_buffer_allocator.hpp"

namespace rhi::vulkan {
	VulkanCommandList::VulkanCommandList(VulkanDevice* device, CommandType type, const std::string& name)
	{
		m_Device = device;
		m_DebugName = name;
		m_CommandType = type;
	}

	VulkanCommandList::~VulkanCommandList()
	{
		((VulkanDevice*)m_Device)->enqueueDeletion(m_CommandPool);
	}

	bool VulkanCommandList::create()
	{
		VulkanDevice* device = (VulkanDevice*)m_Device;
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };

		switch (m_CommandType) {
		case CommandType::Graphics:
			createInfo.queueFamilyIndex = device->getGraphicsQueueIndex();
			m_Queue = device->getGraphicsQueue();
			break;
		case CommandType::Compute:
			createInfo.queueFamilyIndex = device->getComputeQueueIndex();
			m_Queue = device->getComputeQueue();
			break;
		case CommandType::Copy:
			createInfo.queueFamilyIndex = device->getCopyQueueIndex();
			m_Queue = device->getCopyQueue();
			break;
		}

		VK_CHECK_RETURN(vkCreateCommandPool(device->getDevice(), &createInfo, nullptr, &m_CommandPool), false, "Command pool creation failed!");

		setDebugName(device->getDevice(), VK_OBJECT_TYPE_COMMAND_POOL, m_CommandPool, m_DebugName.c_str());
		return true;
	}

	void VulkanCommandList::resetAllocator() {
		vkResetCommandPool((VkDevice)m_Device->getHandle(), m_CommandPool, 0);

		for (size_t i = 0; i < m_PendingCommandBuffers.size(); ++i) {
			m_FreeCommandBuffers.push_back(m_PendingCommandBuffers[i]);
		}
		m_PendingCommandBuffers.clear();
	}

	void VulkanCommandList::begin() {
		if (!m_FreeCommandBuffers.empty()) {
			m_CommandBuffer = m_FreeCommandBuffers.back();
			m_FreeCommandBuffers.pop_back();
		}
		else {
			VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			info.commandPool = m_CommandPool;
			info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			info.commandBufferCount = 1;

			vkAllocateCommandBuffers((VkDevice)m_Device->getHandle(), &info, &m_CommandBuffer);
		}

		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
		resetState();
	}

	void VulkanCommandList::end()
	{
		flushBarriers();

		vkEndCommandBuffer(m_CommandBuffer);
		m_PendingCommandBuffers.push_back(m_CommandBuffer);
	}

	void VulkanCommandList::wait(IFence* dstFence, uint64_t value) {
		m_PendingWaits.emplace_back(dstFence, value);
	}

	void VulkanCommandList::signal(IFence* dstFence, uint64_t value) {
		m_PendingSignals.emplace_back(dstFence, value);
	}

	void VulkanCommandList::present(ISwapchain* dstSwapchain) {
		m_PendingSwapchains.push_back(dstSwapchain);
	}

	void VulkanCommandList::submit()
	{
		((VulkanDevice*)m_Device)->flushLayoutTransition(m_CommandType);
		std::vector<VkSemaphore> waitSemaphores;
		std::vector<VkSemaphore> signalSemaphores;
		std::vector<uint64_t> waitValues;
		std::vector<uint64_t> signalValues;
		std::vector<VkPipelineStageFlags> waitStages;

		for (const auto& wait : m_PendingWaits) {
			waitSemaphores.push_back(static_cast<VkSemaphore>(wait.first->getHandle()));
			waitStages.push_back(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
			waitValues.push_back(wait.second);
		}
		m_PendingWaits.clear();

		for (const auto& signal : m_PendingSignals) {
			signalSemaphores.push_back(static_cast<VkSemaphore>(signal.first->getHandle()));
			signalValues.push_back(signal.second);
		}
		m_PendingSignals.clear();

		for (ISwapchain* swapchain : m_PendingSwapchains) {
			VulkanSwapchain* vulkanSwapchain = static_cast<VulkanSwapchain*>(swapchain);

			waitSemaphores.push_back(vulkanSwapchain->getAcquireSemaphore());
			waitStages.push_back(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
			signalSemaphores.push_back(vulkanSwapchain->getPresentSemaphore());

			waitValues.push_back(0);
			signalValues.push_back(0);
		}

		VkTimelineSemaphoreSubmitInfo timelineInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
		timelineInfo.waitSemaphoreValueCount = static_cast<uint32_t>(waitValues.size());
		timelineInfo.pWaitSemaphoreValues = waitValues.data();
		timelineInfo.signalSemaphoreValueCount = static_cast<uint32_t>(signalValues.size());
		timelineInfo.pSignalSemaphoreValues = signalValues.data();

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.pNext = &timelineInfo;
		submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = waitStages.data();
		submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
		submitInfo.pSignalSemaphores = signalSemaphores.data();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;

		vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);

		for (ISwapchain* swapchain : m_PendingSwapchains) {
			static_cast<VulkanSwapchain*>(swapchain)->present(m_Queue);
		}
		m_PendingSwapchains.clear();
	}
	void VulkanCommandList::resetState() {
		if (m_CommandType == CommandType::Graphics || m_CommandType == CommandType::Compute) {
			auto device = (VulkanDevice*)m_Device;
			VkDescriptorBufferBindingInfoEXT descriptorBuffer[3] = {};
			descriptorBuffer[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptorBuffer[0].address = device->getConstantBufferAllocator()->getGpuAddress();
			descriptorBuffer[0].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			descriptorBuffer[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptorBuffer[1].address = device->getResourceDescriptorAllocator()->getGpuAddress();
			descriptorBuffer[1].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			descriptorBuffer[2].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptorBuffer[2].address = device->getSamplerDescriptorAllocator()->getGpuAddress();
			descriptorBuffer[2].usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;

			vkCmdBindDescriptorBuffersEXT(m_CommandBuffer, 3, descriptorBuffer);

			uint32_t bufferIndices[] = { 1, 2 };
			VkDeviceSize offsets[] = { 0, 0 };

			//vkCmdSetDescriptorBufferOffsetsEXT(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, device->getPipelineLayout(), 1, 2, bufferIndices, offsets);

			if (m_CommandType == CommandType::Graphics)
			{
				vkCmdSetDescriptorBufferOffsetsEXT(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, device->getPipelineLayout(), 1, 2, bufferIndices, offsets);
			}
		}
	}

	void VulkanCommandList::copyBufferToTexture(ITexture* dstTexture, uint32_t mipLevel, uint32_t arraySlice, IBuffer* srcBuffer, uint32_t offset) {
		flushBarriers();

		const TextureDescription& desc = dstTexture->getDescription();

		VkBufferImageCopy2 copy = { VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2 };
		copy.bufferOffset = offset;
		copy.imageSubresource.aspectMask = getVkAspectMask(desc.format);
		copy.imageSubresource.mipLevel = mipLevel;
		copy.imageSubresource.baseArrayLayer = arraySlice;
		copy.imageSubresource.layerCount = 1;
		copy.imageExtent.width = std::max(desc.width >> mipLevel, 1u);
		copy.imageExtent.height = std::max(desc.height >> mipLevel, 1u);
		copy.imageExtent.depth = std::max(desc.depth >> mipLevel, 1u);

		VkCopyBufferToImageInfo2 info = { VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2 };
		info.srcBuffer = static_cast<VkBuffer>(srcBuffer->getHandle());
		info.dstImage = static_cast<VkImage>(dstTexture->getHandle());
		info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		info.regionCount = 1;
		info.pRegions = &copy;

		vkCmdCopyBufferToImage2(m_CommandBuffer, &info);
	}

	void VulkanCommandList::copyTextureToBuffer(IBuffer* dstBuffer, uint32_t offset, ITexture* srcTexture, uint32_t mipLevel, uint32_t arraySlice) {
		flushBarriers();

		const TextureDescription& desc = srcTexture->getDescription();

		VkBufferImageCopy2 copy = { VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2 };
		copy.bufferOffset = offset;
		copy.imageSubresource.aspectMask = getVkAspectMask(desc.format);
		copy.imageSubresource.mipLevel = mipLevel;
		copy.imageSubresource.baseArrayLayer = arraySlice;
		copy.imageSubresource.layerCount = 1;
		copy.imageExtent.width = std::max(desc.width >> mipLevel, 1u);
		copy.imageExtent.height = std::max(desc.height >> mipLevel, 1u);
		copy.imageExtent.depth = std::max(desc.depth >> mipLevel, 1u);

		VkCopyImageToBufferInfo2 info = { VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2 };
		info.srcImage = static_cast<VkImage>(srcTexture->getHandle());
		info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		info.dstBuffer = static_cast<VkBuffer>(dstBuffer->getHandle());
		info.regionCount = 1;
		info.pRegions = &copy;

		vkCmdCopyImageToBuffer2(m_CommandBuffer, &info);
	}

	void VulkanCommandList::copyBuffer(IBuffer* dstBuffer, uint32_t dstOffset, IBuffer* srcBuffer, uint32_t srcOffset, uint32_t size) {
		flushBarriers();

		VkBufferCopy2 copy = { VK_STRUCTURE_TYPE_BUFFER_COPY_2 };
		copy.srcOffset = srcOffset;
		copy.dstOffset = dstOffset;
		copy.size = size;

		VkCopyBufferInfo2 info = { VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2 };
		info.srcBuffer = static_cast<VkBuffer>(srcBuffer->getHandle());
		info.dstBuffer = static_cast<VkBuffer>(dstBuffer->getHandle());
		info.regionCount = 1;
		info.pRegions = &copy;

		vkCmdCopyBuffer2(m_CommandBuffer, &info);
	}

	void VulkanCommandList::copyTexture(ITexture* dstTexture, uint32_t dstMip, uint32_t dstArray, ITexture* srcTexture, uint32_t srcMip, uint32_t srcArray)
	{
		flushBarriers();

		VkImageCopy2 copy = { VK_STRUCTURE_TYPE_IMAGE_COPY_2 };
		copy.srcSubresource.aspectMask = getVkAspectMask(srcTexture->getDescription().format);
		copy.srcSubresource.mipLevel = srcMip;
		copy.srcSubresource.baseArrayLayer = srcArray;
		copy.srcSubresource.layerCount = 1;

		copy.dstSubresource.aspectMask = getVkAspectMask(dstTexture->getDescription().format);
		copy.dstSubresource.mipLevel = dstMip;
		copy.dstSubresource.baseArrayLayer = dstArray;
		copy.dstSubresource.layerCount = 1;

		copy.extent.width = std::max(srcTexture->getDescription().width >> srcMip, 1u);
		copy.extent.height = std::max(srcTexture->getDescription().height >> srcMip, 1u);
		copy.extent.depth = std::max(srcTexture->getDescription().depth >> srcMip, 1u);

		VkCopyImageInfo2 info = { VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2 };
		info.srcImage = (VkImage)srcTexture->getHandle();
		info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		info.dstImage = (VkImage)dstTexture->getHandle();
		info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		info.regionCount = 1;
		info.pRegions = &copy;

		vkCmdCopyImage2(m_CommandBuffer, &info);
	}

	void VulkanCommandList::clearStorageBuffer(IResource* resource, IDescriptor* storage, const float* clearValue)
	{
		//const UnorderedAccessDescriptorDescription& desc = ((VulkanUnorderedAccessDescriptor*)storage)->getDescription();
	}

	void VulkanCommandList::clearStorageBuffer(IResource* resource, IDescriptor* storage, const uint32_t* clearValue)
	{
	}

	void VulkanCommandList::textureBarrier(ITexture* texture, ResourceAccessFlags accessBefore, ResourceAccessFlags accessAfter) {
		VkImageMemoryBarrier2 barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		barrier.image = static_cast<VkImage>(texture->getHandle());
		barrier.srcStageMask = getVkStageMask(accessBefore);
		barrier.dstStageMask = getVkStageMask(accessAfter);
		barrier.srcAccessMask = getVkAccessMask(accessBefore);
		barrier.dstAccessMask = getVkAccessMask(accessAfter);
		barrier.oldLayout = getVkImageLayout(accessBefore);
		barrier.newLayout = anySet(accessAfter, ResourceAccessFlags::Discard) ? barrier.oldLayout : getVkImageLayout(accessAfter);
		barrier.subresourceRange.aspectMask = getVkAspectMask(texture->getDescription().format);

		//Using all subresources
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		m_ImageBarriers.push_back(barrier);
	}

	void VulkanCommandList::bufferBarrier(IBuffer* buffer, ResourceAccessFlags accessBefore, ResourceAccessFlags accessAfter) {
		VkBufferMemoryBarrier2 barrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
		barrier.buffer = static_cast<VkBuffer>(buffer->getHandle());
		barrier.offset = 0;
		barrier.size = buffer->getDescription().size;
		barrier.srcStageMask = getVkStageMask(accessBefore);
		barrier.dstStageMask = getVkStageMask(accessAfter);
		barrier.srcAccessMask = getVkAccessMask(accessBefore);
		barrier.dstAccessMask = getVkAccessMask(accessAfter);

		m_BufferBarriers.push_back(barrier);
	}

	void VulkanCommandList::globalBarrier(ResourceAccessFlags accessBefore, ResourceAccessFlags accessAfter) {
		VkMemoryBarrier2 barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER_2 };
		barrier.srcStageMask = getVkStageMask(accessBefore);
		barrier.dstStageMask = getVkStageMask(accessAfter);
		barrier.srcAccessMask = getVkAccessMask(accessBefore);
		barrier.dstAccessMask = getVkAccessMask(accessAfter);

		m_MemoryBarriers.push_back(barrier);
	}

	void VulkanCommandList::flushBarriers() {
		if (!m_MemoryBarriers.empty() || !m_BufferBarriers.empty() || !m_ImageBarriers.empty()) {
			VkDependencyInfo info = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
			info.memoryBarrierCount = static_cast<uint32_t>(m_MemoryBarriers.size());
			info.pMemoryBarriers = m_MemoryBarriers.data();
			info.bufferMemoryBarrierCount = static_cast<uint32_t>(m_BufferBarriers.size());
			info.pBufferMemoryBarriers = m_BufferBarriers.data();
			info.imageMemoryBarrierCount = static_cast<uint32_t>(m_ImageBarriers.size());
			info.pImageMemoryBarriers = m_ImageBarriers.data();

			vkCmdPipelineBarrier2(m_CommandBuffer, &info);

			m_MemoryBarriers.clear();
			m_BufferBarriers.clear();
			m_ImageBarriers.clear();
		}
	}

	void VulkanCommandList::beginRenderPass(const RenderPassDescription& renderPass) {
		flushBarriers();

		VkRenderingAttachmentInfo colorAttachments[8] = {};
		VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		VkRenderingAttachmentInfo stencilAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		uint32_t width = 0;
		uint32_t height = 0;

		uint32_t numberAttachments = 0;
		for (uint32_t i = 0; i < 8; ++i) {
			colorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

			if (renderPass.color[i].texture) {
				if (width == 0) {
					width = renderPass.color[i].texture->getDescription().width;
				}

				if (height == 0) {
					height = renderPass.color[i].texture->getDescription().height;
				}

				SE_ASSERT_NOMSG(width == renderPass.color[i].texture->getDescription().width);
				SE_ASSERT_NOMSG(height == renderPass.color[i].texture->getDescription().height);

				colorAttachments[i].imageView = static_cast<VulkanTexture*>(renderPass.color[i].texture)->getRenderView(renderPass.color[i].mipSlice, renderPass.color[i].arraySlice);
				colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorAttachments[i].loadOp = toVkLoadOp(renderPass.color[i].loadOp);
				colorAttachments[i].storeOp = toVkStoreOp(renderPass.color[i].storeOp);
				memcpy(colorAttachments[i].clearValue.color.float32, renderPass.color[i].clearColor, sizeof(float) * 4);

				numberAttachments++;
			}
		}

		if (renderPass.depth.texture != nullptr) {
			if (width == 0) {
				width = renderPass.depth.texture->getDescription().width;
			}

			if (height == 0) {
				height = renderPass.depth.texture->getDescription().height;
			}

			SE_ASSERT_NOMSG(width == renderPass.depth.texture->getDescription().width);
			SE_ASSERT_NOMSG(height == renderPass.depth.texture->getDescription().height);

			depthAttachment.imageView = static_cast<VulkanTexture*>(renderPass.depth.texture)->getRenderView(renderPass.depth.mipSlice, renderPass.depth.arraySlice);
			depthAttachment.imageLayout = renderPass.depth.readOnly ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachment.loadOp = toVkLoadOp(renderPass.depth.loadOp);
			depthAttachment.storeOp = toVkStoreOp(renderPass.depth.storeOp);

			if (isStencilFormat(renderPass.depth.texture->getDescription().format)) {
				stencilAttachment.imageView = static_cast<VulkanTexture*>(renderPass.depth.texture)->getRenderView(renderPass.depth.mipSlice, renderPass.depth.arraySlice);
				stencilAttachment.imageLayout = renderPass.depth.readOnly ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				stencilAttachment.loadOp = toVkLoadOp(renderPass.depth.stencilLoadOp);
				stencilAttachment.storeOp = toVkStoreOp(renderPass.depth.stencilStoreOp);
			}
		}

		VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
		renderingInfo.renderArea.extent.width = width;
		renderingInfo.renderArea.extent.height = height;
		renderingInfo.layerCount = 1;
		renderingInfo.viewMask = 0;
		renderingInfo.colorAttachmentCount = numberAttachments;
		renderingInfo.pColorAttachments = colorAttachments;

		if (depthAttachment.imageView != VK_NULL_HANDLE) {
			renderingInfo.pDepthAttachment = &depthAttachment;
		}

		if (stencilAttachment.imageView != VK_NULL_HANDLE) {
			renderingInfo.pStencilAttachment = &stencilAttachment;
		}

		vkCmdBeginRendering(m_CommandBuffer, &renderingInfo);
		setViewport(0, 0, width, height);
	}

	void VulkanCommandList::endRenderPass() {
		vkCmdEndRendering(m_CommandBuffer);
	}

	void VulkanCommandList::bindPipeline(IPipelineState* pipeline) {
		VkPipelineBindPoint bindPoint = pipeline->getType() == PipelineType::Compute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
		vkCmdBindPipeline(m_CommandBuffer, bindPoint, static_cast<VkPipeline>(pipeline->getHandle()));
	}

	void VulkanCommandList::setStencilReference(uint8_t reference) {
		vkCmdSetStencilReference(m_CommandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, reference);
	}

	void VulkanCommandList::setBlendFactor(const float* blendFactor) {
		vkCmdSetBlendConstants(m_CommandBuffer, blendFactor);
	}

	void VulkanCommandList::setIndexBuffer(IBuffer* buffer, uint32_t offset, Format format) {
		VkIndexType indexType = (format == Format::R16_UINT) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
		vkCmdBindIndexBuffer(m_CommandBuffer, static_cast<VkBuffer>(buffer->getHandle()), offset, indexType);
	}

	void VulkanCommandList::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
		VkViewport viewport;
		viewport.x = static_cast<float>(x);
		viewport.y = static_cast<float>(height) - static_cast<float>(y);
		viewport.width = static_cast<float>(width);
		viewport.height = -static_cast<float>(height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
		setScissorRect(x, y, width, height);
	}

	void VulkanCommandList::setScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
		VkRect2D scissor;
		scissor.offset.x = static_cast<int32_t>(x);
		scissor.offset.y = static_cast<int32_t>(y);
		scissor.extent.width = width;
		scissor.extent.height = height;

		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
	}

	void VulkanCommandList::setGraphicsPushConstants(uint32_t slot, const void* data, size_t dataSize)
	{
		if (slot == 0)
		{
			SE_ASSERT_NOMSG(dataSize <= SE_MAX_PUSH_CONSTANTS * sizeof(uint32_t));
			memcpy(m_GraphicsConstants.ubv0, data, dataSize);
		}
		else
		{
			SE_ASSERT_NOMSG(slot < SE_MAX_UBV_BINDINGS);
			VkDeviceAddress gpuAddress = ((VulkanDevice*)m_Device)->allocateUniformBuffer(data, dataSize);

			if (slot == 1)
			{
				m_GraphicsConstants.ubv1.address = gpuAddress;
				m_GraphicsConstants.ubv1.range = dataSize;
			}
			else
			{
				m_GraphicsConstants.ubv2.address = gpuAddress;
				m_GraphicsConstants.ubv2.range = dataSize;
			}
		}

		m_GraphicsConstants.needsUpdate = true;
	}

	void VulkanCommandList::setComputePushConstants(uint32_t slot, const void* data, size_t dataSize)
	{
		if (slot == 0)
		{
			SE_ASSERT_NOMSG(dataSize <= SE_MAX_PUSH_CONSTANTS * sizeof(uint32_t));
			memcpy(m_ComputeConstants.ubv0, data, dataSize);
		}
		else
		{
			SE_ASSERT_NOMSG(slot < SE_MAX_UBV_BINDINGS);
			VkDeviceAddress gpuAddress = ((VulkanDevice*)m_Device)->allocateUniformBuffer(data, dataSize);

			if (slot == 1)
			{
				m_ComputeConstants.ubv1.address = gpuAddress;
				m_ComputeConstants.ubv1.range = dataSize;
			}
			else
			{
				m_ComputeConstants.ubv2.address = gpuAddress;
				m_ComputeConstants.ubv2.range = dataSize;
			}
		}

		m_GraphicsConstants.needsUpdate = true;
	}

	void VulkanCommandList::draw(uint32_t vertexCount, uint32_t instanceCount) {
		updateGraphicsDescriptorBuffer();
		vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, 0, 0);
	}

	void VulkanCommandList::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset) {
		updateGraphicsDescriptorBuffer();
		vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, indexOffset, 0, 0);
	}

	void VulkanCommandList::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
		flushBarriers();
		updateComputeDescriptorBuffer();
		vkCmdDispatch(m_CommandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanCommandList::updateGraphicsDescriptorBuffer()
	{
		if (m_GraphicsConstants.needsUpdate)
		{
			VulkanDevice* device = (VulkanDevice*)m_Device;
			VkDeviceSize cbvDescriptorOffset = device->allocateUniformBufferDescriptor(m_GraphicsConstants.ubv0, m_GraphicsConstants.ubv1, m_GraphicsConstants.ubv2);

			uint32_t bufferIndices[] = { 0 };
			VkDeviceSize offsets[] = { cbvDescriptorOffset };

			vkCmdSetDescriptorBufferOffsetsEXT(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, device->getPipelineLayout(), 0, 1, bufferIndices, offsets);

			m_GraphicsConstants.needsUpdate = false;
		}
	}

	void VulkanCommandList::updateComputeDescriptorBuffer()
	{
		if (m_ComputeConstants.needsUpdate)
		{
			VulkanDevice* device = (VulkanDevice*)m_Device;
			VkDeviceSize cbvDescriptorOffset = device->allocateUniformBufferDescriptor(m_ComputeConstants.ubv0, m_ComputeConstants.ubv1, m_ComputeConstants.ubv2);

			uint32_t bufferIndices[] = { 0 };
			VkDeviceSize offsets[] = { cbvDescriptorOffset };

			vkCmdSetDescriptorBufferOffsetsEXT(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, device->getPipelineLayout(), 0, 1, bufferIndices, offsets);

			m_ComputeConstants.needsUpdate = false;
		}
	}

	//void VulkanCommandList::drawIndirect(Buffer* buffer, uint32_t offset) {
	//	updateGraphicsDescriptorBuffer();
	//	vkCmdDrawIndirect(m_CommandBuffer, static_cast<VkBuffer>(buffer->getHandle()), offset, 1, 0);
	//}

	//void VulkanCommandList::drawIndexedIndirect(Buffer* buffer, uint32_t offset) {
	//	updateGraphicsDescriptorBuffer();
	//	vkCmdDrawIndexedIndirect(m_CommandBuffer, static_cast<VkBuffer>(buffer->getHandle()), offset, 1, 0);
	//}

	//void VulkanCommandList::dispatchIndirect(Buffer* buffer, uint32_t offset) {
	//	flushBarriers();
	//	updateComputeDescriptorBuffer();
	//	vkCmdDispatchIndirect(m_CommandBuffer, static_cast<VkBuffer>(buffer->getHandle()), offset);
	//}

	//void VulkanCommandList::dispatchMesh(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
	//	updateGraphicsDescriptorBuffer();
	//	vkCmdDrawMeshTasksEXT(m_CommandBuffer, groupCountX, groupCountY, groupCountZ);
	//}

	//void VulkanCommandList::dispatchMeshIndirect(Buffer* buffer, uint32_t offset) {
	//	updateGraphicsDescriptorBuffer();
	//	vkCmdDrawMeshTasksIndirectEXT(m_CommandBuffer, static_cast<VkBuffer>(buffer->getHandle()), offset, 1, 0);
	//}

	//void VulkanCommandList::multiDrawIndirect(uint32_t maxCount, Buffer* argsBuffer, uint32_t argsBufferOffset, Buffer* countBuffer, uint32_t countBufferOffset) {
	//	updateGraphicsDescriptorBuffer();
	//	vkCmdDrawIndirectCount(m_CommandBuffer, static_cast<VkBuffer>(argsBuffer->getHandle()), argsBufferOffset,
	//		static_cast<VkBuffer>(countBuffer->getHandle()), countBufferOffset, maxCount, sizeof(DrawCommand));
	//}

	//void VulkanCommandList::multiDrawIndexedIndirect(uint32_t maxCount, Buffer* argsBuffer, uint32_t argsBufferOffset, Buffer* countBuffer, uint32_t countBufferOffset) {
	//	updateGraphicsDescriptorBuffer();
	//	vkCmdDrawIndexedIndirectCount(m_CommandBuffer, static_cast<VkBuffer>(argsBuffer->getHandle()), argsBufferOffset,
	//		static_cast<VkBuffer>(countBuffer->getHandle()), countBufferOffset, maxCount, sizeof(DrawIndexedCommand));
	//}

	//void VulkanCommandList::clearStorageBuffer(Resource* resource, DescriptorSet* uav, const float* clearValue) {
	//	const UnorderedAccessDescriptorDesc& desc = static_cast<VulkanUnorderedAccessDescriptor*>(uav)->getDesc();
	//	::clearUAV(this, resource, uav, desc, clearValue);
	//}

	//void VulkanCommandList::clearStorageBuffer(Resource* resource, DescriptorSet* uav, const uint32_t* clearValue) {
	//	const UnorderedAccessDescriptorDesc& desc = static_cast<VulkanUnorderedAccessDescriptor*>(uav)->getDesc();
	//	::clearUAV(this, resource, uav, desc, clearValue);
	//}

	void VulkanCommandList::writeBuffer(IBuffer* buffer, uint32_t offset, uint32_t data) {
		flushBarriers();
		vkCmdUpdateBuffer(m_CommandBuffer, static_cast<VkBuffer>(buffer->getHandle()), offset, sizeof(uint32_t), &data);
	}

	//void VulkanCommandList::beginEvent(const std::string& eventName) {
	//	VkDebugUtilsLabelEXT labelInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
	//	labelInfo.pLabelName = eventName.c_str();
	//	vkCmdBeginDebugUtilsLabelEXT(m_CommandBuffer, &labelInfo);
	//}

	//void VulkanCommandList::endEvent() {
	//	vkCmdEndDebugUtilsLabelEXT(m_CommandBuffer);
	//}

	//void VulkanCommandList::updateGraphicsDescriptorBuffer() {
	//	VkDeviceSize cbvDescriptorOffset = m_Device->allocateConstantBufferDescriptor(m_GraphicsConstants.cbv0, m_GraphicsConstants.cbv1, m_GraphicsConstants.cbv2);

	//	uint32_t bufferIndices[] = { 0, 1, 2 };
	//	VkDeviceSize offsets[] = { cbvDescriptorOffset, 0, 0 };
	//	vkCmdSetDescriptorBufferOffsetsEXT(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Device->getPipelineLayout(), 0, 3, bufferIndices, offsets);
	//}

	//void VulkanCommandList::updateComputeDescriptorBuffer() {
	//	VkDeviceSize cbvDescriptorOffset = m_Device->allocateConstantBufferDescriptor(m_ComputeConstants.cbv0, m_ComputeConstants.cbv1, m_ComputeConstants.cbv2);

	//	uint32_t bufferIndices[] = { 0, 1, 2 };
	//	VkDeviceSize offsets[] = { cbvDescriptorOffset, 0, 0 };
	//	vkCmdSetDescriptorBufferOffsetsEXT(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ((VulkanDevice*)m_Device)->p(), 0, 3, bufferIndices, offsets);
	//}
}