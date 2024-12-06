#pragma once
#include "vulkan_core.hpp"

#include "../types.hpp"

namespace rhi::vulkan
{
	void validateSamplerDescription(const SamplerDescription& desc) {
		SE_ASSERT(desc.maxAnisotropy >= 1.0f && desc.maxAnisotropy <= MaxSamplerAnisotropy,
			"Invalid anisotropy value: {}", desc.maxAnisotropy);
		SE_ASSERT(desc.minLod <= desc.maxLod,
			"Invalid LOD range: min {} > max {}", desc.minLod, desc.maxLod);
	}

	VmaMemoryUsage translateMemoryType(MemoryType type) {
		switch (type) {
		case MemoryType::GpuOnly:
			return VMA_MEMORY_USAGE_GPU_ONLY;
		case MemoryType::CpuToGpu:
			return VMA_MEMORY_USAGE_CPU_TO_GPU;
		case MemoryType::GpuToCpu:
			return VMA_MEMORY_USAGE_GPU_TO_CPU;
		case MemoryType::CpuOnly:
			return VMA_MEMORY_USAGE_CPU_ONLY;
		default:
			SE::LogError("Invalid memory type: {}", static_cast<int>(type));
			return VMA_MEMORY_USAGE_UNKNOWN;
		}
	}
	VkFormat toVkFormat(Format format, bool sRGB) {
		switch (format) {
		case Format::R8G8B8A8_UNORM:
			return sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
		case Format::B8G8R8A8_UNORM:
			return sRGB ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;
		case Format::R16G16B16A16_SFLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Format::R32G32B32A32_SFLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Format::D32_SFLOAT:
			return VK_FORMAT_D32_SFLOAT;
		case Format::R8_UNORM:
			return VK_FORMAT_R8_UNORM;
		case Format::R8_SNORM:
			return VK_FORMAT_R8_SNORM;
		case Format::R8_UINT:
			return VK_FORMAT_R8_UINT;
		case Format::R8_SINT:
			return VK_FORMAT_R8_SINT;
		case Format::R8G8_UNORM:
			return VK_FORMAT_R8G8_UNORM;
		case Format::R8G8_SNORM:
			return VK_FORMAT_R8G8_SNORM;
		case Format::R8G8_UINT:
			return VK_FORMAT_R8G8_UINT;
		case Format::R8G8_SINT:
			return VK_FORMAT_R8G8_SINT;
		case Format::R8G8B8_UNORM:
			return sRGB ? VK_FORMAT_R8G8B8_SRGB : VK_FORMAT_R8G8B8_UNORM;
		case Format::R8G8B8_SNORM:
			return VK_FORMAT_R8G8B8_SNORM;
		case Format::R8G8B8_UINT:
			return VK_FORMAT_R8G8B8_UINT;
		case Format::R8G8B8_SINT:
			return VK_FORMAT_R8G8B8_SINT;
		case Format::R8G8B8A8_SNORM:
			return VK_FORMAT_R8G8B8A8_SNORM;
		case Format::R8G8B8A8_UINT:
			return VK_FORMAT_R8G8B8A8_UINT;
		case Format::R8G8B8A8_SINT:
			return VK_FORMAT_R8G8B8A8_SINT;
		case Format::B8G8R8A8_SNORM:
			return VK_FORMAT_B8G8R8A8_SNORM;
		case Format::B8G8R8A8_UINT:
			return VK_FORMAT_B8G8R8A8_UINT;
		case Format::B8G8R8A8_SINT:
			return VK_FORMAT_B8G8R8A8_SINT;
		case Format::R16_UNORM:
			return VK_FORMAT_R16_UNORM;
		case Format::R16_SNORM:
			return VK_FORMAT_R16_SNORM;
		case Format::R16_UINT:
			return VK_FORMAT_R16_UINT;
		case Format::R16_SINT:
			return VK_FORMAT_R16_SINT;
		case Format::R16_SFLOAT:
			return VK_FORMAT_R16_SFLOAT;
		case Format::R16G16_UNORM:
			return VK_FORMAT_R16G16_UNORM;
		case Format::R16G16_SNORM:
			return VK_FORMAT_R16G16_SNORM;
		case Format::R16G16_UINT:
			return VK_FORMAT_R16G16_UINT;
		case Format::R16G16_SINT:
			return VK_FORMAT_R16G16_SINT;
		case Format::R16G16_SFLOAT:
			return VK_FORMAT_R16G16_SFLOAT;
		case Format::R16G16B16_UNORM:
			return VK_FORMAT_R16G16B16_UNORM;
		case Format::R16G16B16_SNORM:
			return VK_FORMAT_R16G16B16_SNORM;
		case Format::R16G16B16_UINT:
			return VK_FORMAT_R16G16B16_UINT;
		case Format::R16G16B16_SINT:
			return VK_FORMAT_R16G16B16_SINT;
		case Format::R16G16B16_SFLOAT:
			return VK_FORMAT_R16G16B16_SFLOAT;
		case Format::R16G16B16A16_UNORM:
			return VK_FORMAT_R16G16B16A16_UNORM;
		case Format::R16G16B16A16_SNORM:
			return VK_FORMAT_R16G16B16A16_SNORM;
		case Format::R16G16B16A16_UINT:
			return VK_FORMAT_R16G16B16A16_UINT;
		case Format::R16G16B16A16_SINT:
			return VK_FORMAT_R16G16B16A16_SINT;
		case Format::R32_UINT:
			return VK_FORMAT_R32_UINT;
		case Format::R32_SINT:
			return VK_FORMAT_R32_SINT;
		case Format::R32G32_UINT:
			return VK_FORMAT_R32G32_UINT;
		case Format::R32G32_SINT:
			return VK_FORMAT_R32G32_SINT;
		case Format::R32G32B32_UINT:
			return VK_FORMAT_R32G32B32_UINT;
		case Format::R32G32B32_SINT:
			return VK_FORMAT_R32G32B32_SINT;
		case Format::R32G32B32A32_UINT:
			return VK_FORMAT_R32G32B32A32_UINT;
		case Format::R32G32B32A32_SINT:
			return VK_FORMAT_R32G32B32A32_SINT;
		case Format::R4G4_UNORM_PACK8:
			return VK_FORMAT_R4G4_UNORM_PACK8;
		case Format::R5G6B5_UNORM_PACK16:
			return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case Format::B5G6R5_UNORM_PACK16:
			return VK_FORMAT_B5G6R5_UNORM_PACK16;
		case Format::R5G5B5A1_UNORM_PACK16:
			return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
		case Format::B5G5R5A1_UNORM_PACK16:
			return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
		case Format::A1R5G5B5_UNORM_PACK16:
			return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case Format::R4G4B4A4_UNORM_PACK16:
			return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
		case Format::B4G4R4A4_UNORM_PACK16:
			return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
		case Format::D16_UNORM:
			return VK_FORMAT_D16_UNORM;
		case Format::D24_UNORM_S8_UINT:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case Format::D32_SFLOAT_S8_UINT:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case Format::S8_UINT:
			return VK_FORMAT_S8_UINT;
		case Format::BC1_UNORM:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case Format::BC1_SRGB:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case Format::BC2_UNORM:
			return VK_FORMAT_BC2_UNORM_BLOCK;
		case Format::BC2_SRGB:
			return VK_FORMAT_BC2_SRGB_BLOCK;
		case Format::BC3_UNORM:
			return VK_FORMAT_BC3_UNORM_BLOCK;
		case Format::BC3_SRGB:
			return VK_FORMAT_BC3_SRGB_BLOCK;
		case Format::BC4_UNORM:
			return VK_FORMAT_BC4_UNORM_BLOCK;
		case Format::BC4_SNORM:
			return VK_FORMAT_BC4_SNORM_BLOCK;
		case Format::BC5_UNORM:
			return VK_FORMAT_BC5_UNORM_BLOCK;
		case Format::BC5_SNORM:
			return VK_FORMAT_BC5_SNORM_BLOCK;
		case Format::BC6H_UFLOAT:
			return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case Format::BC6H_SFLOAT:
			return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case Format::BC7_UNORM:
			return VK_FORMAT_BC7_UNORM_BLOCK;
		case Format::BC7_SRGB:
			return VK_FORMAT_BC7_SRGB_BLOCK;
		case Format::ASTC_4x4_UNORM:
			return sRGB ? VK_FORMAT_ASTC_4x4_SRGB_BLOCK : VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
		case Format::ASTC_5x5_UNORM:
			return sRGB ? VK_FORMAT_ASTC_5x5_SRGB_BLOCK : VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
		case Format::ASTC_6x6_UNORM:
			return sRGB ? VK_FORMAT_ASTC_6x6_SRGB_BLOCK : VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
		case Format::ASTC_8x8_UNORM:
			return sRGB ? VK_FORMAT_ASTC_8x8_SRGB_BLOCK : VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
		case Format::ASTC_10x10_UNORM:
			return sRGB ? VK_FORMAT_ASTC_10x10_SRGB_BLOCK : VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
		case Format::ASTC_12x12_UNORM:
			return sRGB ? VK_FORMAT_ASTC_12x12_SRGB_BLOCK : VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
		case Format::ETC2_R8G8B8_UNORM:
			return sRGB ? VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK : VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
		case Format::ETC2_R8G8B8A1_UNORM:
			return sRGB ? VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK : VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
		case Format::ETC2_R8G8B8A8_UNORM:
			return sRGB ? VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK : VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
		case Format::EAC_R11_UNORM:
			return VK_FORMAT_EAC_R11_UNORM_BLOCK;
		case Format::EAC_R11_SNORM:
			return VK_FORMAT_EAC_R11_SNORM_BLOCK;
		case Format::EAC_R11G11_UNORM:
			return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
		case Format::EAC_R11G11_SNORM:
			return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
		case Format::YUV420_8BIT:
			return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
		case Format::YUV420_10BIT:
			return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;
		case Format::YUV422_8BIT:
			return VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM;
		case Format::YUV422_10BIT:
			return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;
		case Format::YUV444_8BIT:
			return VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM;
		case Format::YUV444_10BIT:
			return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;
		default:
			SE::LogWarn("Unknown format: {}, falling back to UNDEFINED", static_cast<int>(format));
			return VK_FORMAT_UNDEFINED;
		}
	}

	VkImageAspectFlags getVkAspectMask(Format format) {
		switch (format) {
			// Depth-only formats
		case Format::D16_UNORM:
		case Format::D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;

			// Depth-stencil formats
		case Format::D24_UNORM_S8_UINT:
		case Format::D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

			// Stencil-only format
		case Format::S8_UINT:
			return VK_IMAGE_ASPECT_STENCIL_BIT;

			// Color formats
		case Format::R8_UNORM:
		case Format::R8_SNORM:
		case Format::R8_UINT:
		case Format::R8_SINT:
		case Format::R8G8_UNORM:
		case Format::R8G8_SNORM:
		case Format::R8G8_UINT:
		case Format::R8G8_SINT:
		case Format::R8G8B8_UNORM:
		case Format::R8G8B8_SNORM:
		case Format::R8G8B8_UINT:
		case Format::R8G8B8_SINT:
		case Format::R8G8B8A8_UNORM:
		case Format::R8G8B8A8_SNORM:
		case Format::R8G8B8A8_UINT:
		case Format::R8G8B8A8_SINT:
		case Format::R8G8B8A8_SRGB:
		case Format::B8G8R8A8_UNORM:
		case Format::B8G8R8A8_SNORM:
		case Format::B8G8R8A8_UINT:
		case Format::B8G8R8A8_SINT:
		case Format::B8G8R8A8_SRGB:
		case Format::R16_UNORM:
		case Format::R16_SNORM:
		case Format::R16_UINT:
		case Format::R16_SINT:
		case Format::R16_SFLOAT:
		case Format::R16G16_UNORM:
		case Format::R16G16_SNORM:
		case Format::R16G16_UINT:
		case Format::R16G16_SINT:
		case Format::R16G16_SFLOAT:
		case Format::R16G16B16_UNORM:
		case Format::R16G16B16_SNORM:
		case Format::R16G16B16_UINT:
		case Format::R16G16B16_SINT:
		case Format::R16G16B16_SFLOAT:
		case Format::R16G16B16A16_UNORM:
		case Format::R16G16B16A16_SNORM:
		case Format::R16G16B16A16_UINT:
		case Format::R16G16B16A16_SINT:
		case Format::R16G16B16A16_SFLOAT:
		case Format::R32_UINT:
		case Format::R32_SINT:
		case Format::R32_SFLOAT:
		case Format::R32G32_UINT:
		case Format::R32G32_SINT:
		case Format::R32G32_SFLOAT:
		case Format::R32G32B32_UINT:
		case Format::R32G32B32_SINT:
		case Format::R32G32B32_SFLOAT:
		case Format::R32G32B32A32_UINT:
		case Format::R32G32B32A32_SINT:
		case Format::R32G32B32A32_SFLOAT:
		case Format::BC1_UNORM:
		case Format::BC1_SRGB:
		case Format::BC2_UNORM:
		case Format::BC2_SRGB:
		case Format::BC3_UNORM:
		case Format::BC3_SRGB:
		case Format::BC4_UNORM:
		case Format::BC4_SNORM:
		case Format::BC5_UNORM:
		case Format::BC5_SNORM:
		case Format::BC6H_UFLOAT:
		case Format::BC6H_SFLOAT:
		case Format::BC7_UNORM:
		case Format::BC7_SRGB:
		case Format::ASTC_4x4_UNORM:
		case Format::ASTC_4x4_SRGB:
		case Format::ASTC_5x5_UNORM:
		case Format::ASTC_5x5_SRGB:
		case Format::ASTC_6x6_UNORM:
		case Format::ASTC_6x6_SRGB:
		case Format::ASTC_8x8_UNORM:
		case Format::ASTC_8x8_SRGB:
		case Format::ASTC_10x10_UNORM:
		case Format::ASTC_10x10_SRGB:
		case Format::ASTC_12x12_UNORM:
		case Format::ASTC_12x12_SRGB:
		case Format::ETC2_R8G8B8_UNORM:
		case Format::ETC2_R8G8B8_SRGB:
		case Format::ETC2_R8G8B8A1_UNORM:
		case Format::ETC2_R8G8B8A1_SRGB:
		case Format::ETC2_R8G8B8A8_UNORM:
		case Format::ETC2_R8G8B8A8_SRGB:
		case Format::EAC_R11_UNORM:
		case Format::EAC_R11_SNORM:
		case Format::EAC_R11G11_UNORM:
		case Format::EAC_R11G11_SNORM:
		case Format::YUV420_8BIT:
		case Format::YUV420_10BIT:
		case Format::YUV422_8BIT:
		case Format::YUV422_10BIT:
		case Format::YUV444_8BIT:
		case Format::YUV444_10BIT:
			return VK_IMAGE_ASPECT_COLOR_BIT;

		default:
			SE::LogWarn("Unknown format: {}, assuming color aspect", static_cast<int>(format));
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}

	VkImageLayout toVkLayout(TextureLayout layout) {
		switch (layout) {
		case TextureLayout::Undefined:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case TextureLayout::RenderTarget:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case TextureLayout::DepthStencil:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case TextureLayout::ShaderResource:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case TextureLayout::ShaderStorage:
			return VK_IMAGE_LAYOUT_GENERAL;
		case TextureLayout::TransferSrc:
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case TextureLayout::TransferDst:
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case TextureLayout::Present:
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		default:
			return VK_IMAGE_LAYOUT_GENERAL;
		}
	}

	VkPipelineStageFlags2 getVkStageMask(ResourceAccessFlags flags) {
		VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;

		if (anySet(flags, ResourceAccessFlags::Present))
			stage |= VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

		if (anySet(flags, ResourceAccessFlags::RenderTarget))
			stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

		if (anySet(flags, ResourceAccessFlags::DepthStencil | ResourceAccessFlags::DepthStencilRead))
			stage |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

		if (anySet(flags, ResourceAccessFlags::VertexShaderRead | ResourceAccessFlags::VertexShaderStorage))
			stage |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

		if (anySet(flags, ResourceAccessFlags::PixelShaderRead | ResourceAccessFlags::PixelShaderStorage))
			stage |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		if (anySet(flags, ResourceAccessFlags::ComputeShaderRead | ResourceAccessFlags::ComputeShaderStorage))
			stage |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

		if (anySet(flags, ResourceAccessFlags::IndexBuffer))
			stage |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;

		if (anySet(flags, ResourceAccessFlags::IndirectArgs))
			stage |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;

		if (anySet(flags, ResourceAccessFlags::TransferSrc | ResourceAccessFlags::TransferDst))
			stage |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;

		if (anySet(flags, ResourceAccessFlags::StorageClear))
			stage |= VK_PIPELINE_STAGE_2_TRANSFER_BIT; // Clear operations typically use the transfer stage

		if (anySet(flags, ResourceAccessFlags::ShadingRate))
			stage |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;

		if (anySet(flags, ResourceAccessFlags::AccelerationStructureRead | ResourceAccessFlags::AccelerationStructureWrite))
			stage |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;

		if (anySet(flags, ResourceAccessFlags::Discard))
			stage |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		if (stage == VK_PIPELINE_STAGE_2_NONE)
		{
			stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			SE::LogWarn("VK_PIPELINE_STAGE IS EMPTY. Possible undefined resource access");
		}

		return stage;
	}

	VkCullModeFlags toVkCullMode(CullMode cullMode) {
		switch (cullMode) {
		case CullMode::None:
			return VK_CULL_MODE_NONE;
		case CullMode::Front:
			return VK_CULL_MODE_FRONT_BIT;
		case CullMode::Back:
			return VK_CULL_MODE_BACK_BIT;
		default:
			SE_ASSERT(false, "Unknown CullMode");
			return VK_CULL_MODE_NONE; // Fallback to satisfy return requirement
		}
	}

	VkPipelineRasterizationStateCreateInfo convertRasterizationState(const Rasterizer& rasterizerState) {
		// Assert depthBiasClamp is non-negative
		SE_ASSERT(rasterizerState.depthBiasClamp >= 0.0f, "Depth bias clamp must be non-negative");

		VkPipelineRasterizationStateCreateInfo rasterState = {};
		rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterState.pNext = nullptr;
		rasterState.flags = 0;
		rasterState.depthClampEnable = rasterizerState.depthClip ? VK_FALSE : VK_TRUE;
		rasterState.rasterizerDiscardEnable = VK_FALSE;
		rasterState.polygonMode = rasterizerState.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
		rasterState.cullMode = toVkCullMode(rasterizerState.cullMode);
		rasterState.frontFace = rasterizerState.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
		rasterState.depthBiasEnable = (rasterizerState.depthBias != 0.0f) || (rasterizerState.slopeScaledDepthBias != 0.0f);
		rasterState.depthBiasConstantFactor = rasterizerState.depthBias;
		rasterState.depthBiasClamp = rasterizerState.depthBiasClamp;
		rasterState.depthBiasSlopeFactor = rasterizerState.slopeScaledDepthBias;
		rasterState.lineWidth = 1.0f; // Default line width

		// Assert if conservativeRaster is true; check if it's supported (pseudo-check)
		SE_ASSERT(!rasterizerState.conservativeRaster, "Conservative rasterization is not supported in this configuration");

		return rasterState;
	}

	VkImageLayout getVkImageLayout(ResourceAccessFlags flags) {
		if (anySet(flags, ResourceAccessFlags::Discard))
			return VK_IMAGE_LAYOUT_UNDEFINED;

		if (anySet(flags, ResourceAccessFlags::Present))
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		if (anySet(flags, ResourceAccessFlags::RenderTarget))
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		if (anySet(flags, ResourceAccessFlags::DepthStencil))
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		if (anySet(flags, ResourceAccessFlags::DepthStencilRead))
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		if (anySet(flags, ResourceAccessFlags::ShaderRead))
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		if (anySet(flags, ResourceAccessFlags::ShaderStorage))
			return VK_IMAGE_LAYOUT_GENERAL;

		if (anySet(flags, ResourceAccessFlags::StorageClear))
			return VK_IMAGE_LAYOUT_GENERAL;

		if (anySet(flags, ResourceAccessFlags::TransferDst))
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		if (anySet(flags, ResourceAccessFlags::TransferSrc))
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		if (anySet(flags, ResourceAccessFlags::ShadingRate))
			return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

		SE::LogWarn("Unknown resource access flag for image layout: {}. General layout used as a fallback", static_cast<uint32_t>(flags));
		return VK_IMAGE_LAYOUT_GENERAL;
	}

	VkAccessFlags2 getVkAccessMask(ResourceAccessFlags flags) {
		VkAccessFlags2 access = VK_ACCESS_2_NONE;

		if (anySet(flags, ResourceAccessFlags::Discard)) {
			return access;
		}

		if (anySet(flags, ResourceAccessFlags::RenderTarget))
			access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

		if (anySet(flags, ResourceAccessFlags::DepthStencil))
			access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		if (anySet(flags, ResourceAccessFlags::DepthStencilRead))
			access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

		if (anySet(flags, ResourceAccessFlags::ShaderRead))
			access |= VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT;

		if (anySet(flags, ResourceAccessFlags::ShaderStorage))
			access |= VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;

		if (anySet(flags, ResourceAccessFlags::StorageClear))
			access |= VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;

		if (anySet(flags, ResourceAccessFlags::TransferDst))
			access |= VK_ACCESS_2_TRANSFER_WRITE_BIT;

		if (anySet(flags, ResourceAccessFlags::TransferSrc))
			access |= VK_ACCESS_2_TRANSFER_READ_BIT;

		if (anySet(flags, ResourceAccessFlags::ShadingRate))
			access |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;

		if (anySet(flags, ResourceAccessFlags::IndexBuffer))
			access |= VK_ACCESS_2_INDEX_READ_BIT;

		if (anySet(flags, ResourceAccessFlags::IndirectArgs))
			access |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;

		if (anySet(flags, ResourceAccessFlags::AccelerationStructureRead))
			access |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

		if (anySet(flags, ResourceAccessFlags::AccelerationStructureWrite))
			access |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

		if (access == VK_ACCESS_2_NONE && !anySet(flags, ResourceAccessFlags::Present))
		{
			SE::LogWarn("VK_ACCESS IS EMPTY. Possible undefined resource access");
		}

		return access;
	}

	VkSamplerCreateInfo samplerCreateInfo(const SamplerDescription& desc) {
		validateSamplerDescription(desc);

		VkSamplerCreateInfo info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		info.magFilter = desc.filterMode == FilterMode::Point ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
		info.minFilter = info.magFilter;
		info.mipmapMode = desc.filterMode == FilterMode::Point ?
			VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;

		info.addressModeU = toVkAddressMode(desc.addressU);
		info.addressModeV = toVkAddressMode(desc.addressV);
		info.addressModeW = toVkAddressMode(desc.addressW);
		info.mipLodBias = desc.mipLodBias;
		info.anisotropyEnable = desc.filterMode == FilterMode::Anisotropic;
		info.maxAnisotropy = desc.maxAnisotropy;
		info.minLod = desc.minLod;
		info.maxLod = desc.maxLod;

		return info;
	}

	VkImageCreateInfo toImageCreateInfo(const TextureDescription& desc) {
		VkImageCreateInfo info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

		switch (desc.type) {
		case TextureType::Texture1D:
			info.imageType = VK_IMAGE_TYPE_1D;
			break;
		case TextureType::Texture3D:
			info.imageType = VK_IMAGE_TYPE_3D;
			break;
		default:
			info.imageType = VK_IMAGE_TYPE_2D;
			break;
		}

		info.format = toVkFormat(desc.format);
		info.extent = { desc.width, desc.height, desc.depth };
		info.mipLevels = desc.mipLevels;
		info.arrayLayers = desc.arraySize;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;

		info.usage = 0;
		if (anySet(desc.usage, TextureUsageFlags::RenderTarget))
			info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (anySet(desc.usage, TextureUsageFlags::DepthStencil))
			info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (anySet(desc.usage, TextureUsageFlags::ShaderStorage))
			info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

		if (desc.type == TextureType::TextureCube ||
			desc.type == TextureType::TextureCubeArray) {
			info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			SE_ASSERT(desc.arraySize % 6 == 0, "Cube texture array size must be a multiple of 6");
		}

		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		return info;
	}
	VkPrimitiveTopology toVkPrimitiveTopology(PrimitiveType primitiveType) {
		switch (primitiveType) {
		case PrimitiveType::PointList:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case PrimitiveType::LineList:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case PrimitiveType::LineStrip:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case PrimitiveType::TriangleList:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case PrimitiveType::TriangleStrip:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		default:
			SE_ASSERT(false, "Invalid topology primitive");
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}
	VkSamplerAddressMode toVkAddressMode(AddressMode mode) {
		switch (mode) {
		case AddressMode::Wrap:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case AddressMode::Mirror:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case AddressMode::Clamp:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case AddressMode::Border:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		default:
			SE_ASSERT(false, "Invalid sampler mode");
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	VkAttachmentLoadOp toVkLoadOp(RenderPassLoadOp loadOp) {
		switch (loadOp)
		{
		case RenderPassLoadOp::Load:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		case RenderPassLoadOp::Clear:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case RenderPassLoadOp::DontCare:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		default:
			SE_ASSERT(false, "Load OP unknown");
		}
	}

	VkAttachmentStoreOp toVkStoreOp(RenderPassStoreOp storeOp) {
		switch (storeOp) {
		case RenderPassStoreOp::Store:
			return VK_ATTACHMENT_STORE_OP_STORE;
		case RenderPassStoreOp::DontCare:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		default:
			SE_ASSERT(false, "Store OP unknown");
		}
	}

	VkSamplerAddressMode toVkSamplerAddressMode(AddressMode addressMode) {
		switch (addressMode) {
		case AddressMode::Wrap:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case AddressMode::Mirror:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case AddressMode::Clamp:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case AddressMode::Border:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		default:
			SE_ASSERT(false, "Address Mode unknown");
		}
	}

	VkFilter toVkFilter(FilterMode filter) {
		switch (filter) {
		case FilterMode::Point:
			return VK_FILTER_NEAREST;
		case FilterMode::Bilinear:
		case FilterMode::Trilinear:
		case FilterMode::Anisotropic:
			return VK_FILTER_LINEAR;
		default:
			return VK_FILTER_NEAREST;
		}
	}

	VkImageViewCreateInfo imageViewCreateInfo() {
		VkImageViewCreateInfo info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		return info;
	}

	VkPipelineRasterizationStateCreateInfo toVkPipelineRasterizationStateCreateInfo(const Rasterizer& state) {
		VkPipelineRasterizationStateCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		createInfo.depthClampEnable = !state.depthClip;
		createInfo.polygonMode = state.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
		createInfo.cullMode = toVkCullMode(state.cullMode);
		createInfo.frontFace = state.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
		createInfo.depthBiasEnable = (state.depthBias != 0.0f || state.slopeScaledDepthBias != 0.0f);
		createInfo.depthBiasConstantFactor = state.depthBias;
		createInfo.depthBiasClamp = state.depthBiasClamp;
		createInfo.depthBiasSlopeFactor = state.slopeScaledDepthBias;
		createInfo.lineWidth = 1.0f;

		SE_ASSERT(!state.conservativeRaster, "Conservative rasterization is not supported in this implementation.");

		return createInfo;
	}

	VkCompareOp toVkCompareOp(CompareFunction func) {
		switch (func) {
		case CompareFunction::Never: return VK_COMPARE_OP_NEVER;
		case CompareFunction::Less: return VK_COMPARE_OP_LESS;
		case CompareFunction::Equal: return VK_COMPARE_OP_EQUAL;
		case CompareFunction::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CompareFunction::Greater: return VK_COMPARE_OP_GREATER;
		case CompareFunction::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
		case CompareFunction::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CompareFunction::Always: return VK_COMPARE_OP_ALWAYS;
		default: return VK_COMPARE_OP_MAX_ENUM;
		}
	}

	VkStencilOp toVkStencilOp(StencilOperation stencilOp) {
		switch (stencilOp) {
		case StencilOperation::Keep: return VK_STENCIL_OP_KEEP;
		case StencilOperation::Zero: return VK_STENCIL_OP_ZERO;
		case StencilOperation::Replace: return VK_STENCIL_OP_REPLACE;
		case StencilOperation::IncrementClamp: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case StencilOperation::DecrementClamp: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case StencilOperation::Invert: return VK_STENCIL_OP_INVERT;
		case StencilOperation::IncrementWrap: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case StencilOperation::DecrementWrap: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default: return VK_STENCIL_OP_MAX_ENUM;
		}
	}

	VkStencilOpState toVkStencilOpState(const DepthStencilOperation& state, uint8_t readMask, uint8_t writeMask) {
		VkStencilOpState stencilOpState = {};
		stencilOpState.failOp = toVkStencilOp(state.stencilFail);
		stencilOpState.depthFailOp = toVkStencilOp(state.depthFail);
		stencilOpState.passOp = toVkStencilOp(state.pass);
		stencilOpState.compareOp = toVkCompareOp(state.stencilFunction);
		stencilOpState.compareMask = readMask;
		stencilOpState.writeMask = writeMask;

		return stencilOpState;
	}

	VkPipelineDepthStencilStateCreateInfo toVkPipelineDepthStencilStateCreateInfo(const DepthStencil& state) {
		VkPipelineDepthStencilStateCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		createInfo.depthTestEnable = state.depthTest;
		createInfo.depthWriteEnable = state.depthWrite;
		createInfo.depthCompareOp = toVkCompareOp(state.depthFunction);
		createInfo.stencilTestEnable = state.stencilTest;
		createInfo.front = toVkStencilOpState(state.frontFace, state.stencilReadMask, state.stencilWriteMask);
		createInfo.back = toVkStencilOpState(state.backFace, state.stencilReadMask, state.stencilWriteMask);

		return createInfo;
	}

	VkBlendFactor toVkBlendFactor(BlendFactor blendFactor, bool alpha) {
		switch (blendFactor) {
		case BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
		case BlendFactor::One: return VK_BLEND_FACTOR_ONE;
		case BlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFactor::InvSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFactor::InvSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendFactor::InvDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case BlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFactor::InvDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFactor::SrcAlphaClamp: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case BlendFactor::ConstantFactor: return alpha ? VK_BLEND_FACTOR_CONSTANT_ALPHA : VK_BLEND_FACTOR_CONSTANT_COLOR;
		case BlendFactor::InvConstantFactor: return alpha ? VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA : VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		default: return VK_BLEND_FACTOR_MAX_ENUM;
		}
	}

	VkBlendOp toVkBlendOp(BlendOperation blendOp) {
		switch (blendOp) {
		case BlendOperation::Add: return VK_BLEND_OP_ADD;
		case BlendOperation::Subtract: return VK_BLEND_OP_SUBTRACT;
		case BlendOperation::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BlendOperation::Min: return VK_BLEND_OP_MIN;
		case BlendOperation::Max: return VK_BLEND_OP_MAX;
		default: return VK_BLEND_OP_MAX_ENUM;
		}
	}

	VkPipelineColorBlendStateCreateInfo toVkPipelineColorBlendStateCreateInfo(const Blend* states, VkPipelineColorBlendAttachmentState* vkStates) {
		for (uint32_t i = 0; i < 8; ++i) {
			vkStates[i].blendEnable = states[i].blendEnabled;
			vkStates[i].srcColorBlendFactor = toVkBlendFactor(states[i].colorSource);
			vkStates[i].dstColorBlendFactor = toVkBlendFactor(states[i].colorDestination);
			vkStates[i].colorBlendOp = toVkBlendOp(states[i].colorOperation);
			vkStates[i].srcAlphaBlendFactor = toVkBlendFactor(states[i].alphaSource, true);
			vkStates[i].dstAlphaBlendFactor = toVkBlendFactor(states[i].alphaDestination, true);
			vkStates[i].alphaBlendOp = toVkBlendOp(states[i].alphaOperation);
			vkStates[i].colorWriteMask = static_cast<VkColorComponentFlags>(states[i].writeMask);
		}

		VkPipelineColorBlendStateCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		createInfo.attachmentCount = 8;
		createInfo.pAttachments = vkStates;

		return createInfo;
	}

	VkImageViewType getVkImageViewType(TextureType type) {
		switch (type) {
		case TextureType::Texture1D:
			return VK_IMAGE_VIEW_TYPE_1D;
		case TextureType::Texture2D:
			return VK_IMAGE_VIEW_TYPE_2D;
		case TextureType::Texture2DArray:
			return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		case TextureType::Texture3D:
			return VK_IMAGE_VIEW_TYPE_3D;
		case TextureType::TextureCube:
			return VK_IMAGE_VIEW_TYPE_CUBE;
		case TextureType::TextureCubeArray:
			return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		default:
			SE::LogError("Invalid texture type for view creation");
			return VK_IMAGE_VIEW_TYPE_2D;
		}
	}

	VkImageSubresourceRange getVkSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel,
		uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
		VkImageSubresourceRange range{};
		range.aspectMask = aspectMask;
		range.baseMipLevel = baseMipLevel;
		range.levelCount = levelCount;
		range.baseArrayLayer = baseArrayLayer;
		range.layerCount = layerCount;
		return range;
	}

	VkPipelineRenderingCreateInfo toVkPipelineRenderingCreateInfo(const GraphicsPipelineDescription& pipelineDesc, VkFormat* colorFormats)
	{
		for (uint32_t i = 0; i < 8; ++i)
		{
			colorFormats[i] = toVkFormat(pipelineDesc.renderTargetFormat[i], true);
		}

		VkPipelineRenderingCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		createInfo.colorAttachmentCount = 8;
		createInfo.pColorAttachmentFormats = colorFormats;
		createInfo.depthAttachmentFormat = toVkFormat(pipelineDesc.depthStencilFormat);

		if (pipelineDesc.depthStencilFormat == Format::D32_SFLOAT_S8_UINT)
		{
			createInfo.stencilAttachmentFormat = createInfo.depthAttachmentFormat;
		}

		return createInfo;
	}

	VkComponentMapping getVkComponentMapping(bool swizzleRGB) {
		if (swizzleRGB) {
			return { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G,
					VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
		}
		return { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	}

	bool isStencilFormat(Format format)
	{
		switch (format) {
		case Format::D24_UNORM_S8_UINT:
		case Format::D32_SFLOAT_S8_UINT:
		case Format::S8_UINT:
			return true;
		default:
			return false;
		}
	}
}