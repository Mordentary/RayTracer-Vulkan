#pragma once
#include "vulkan_core.hpp"

#include "../types.hpp"

namespace rhi::vulkan
{
	inline void validateSamplerDescription(const SamplerDescription& desc) {
		SE_ASSERT(desc.maxAnisotropy >= 1.0f && desc.maxAnisotropy <= MaxSamplerAnisotropy,
			"Invalid anisotropy value: {}", desc.maxAnisotropy);
		SE_ASSERT(desc.minLod <= desc.maxLod,
			"Invalid LOD range: min {} > max {}", desc.minLod, desc.maxLod);
	}

	VkBufferUsageFlags translateBufferUsage(BufferUsageFlags usage) {
		VkBufferUsageFlags result = 0;

		if ((usage & BufferUsageFlags::VertexBuffer) != BufferUsageFlags::None) {
			result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}

		if ((usage & BufferUsageFlags::IndexBuffer) != BufferUsageFlags::None) {
			result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}

		if ((usage & BufferUsageFlags::ConstantBuffer) != BufferUsageFlags::None) {
			result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}

		if ((usage & BufferUsageFlags::UnorderedAccess) != BufferUsageFlags::None) {
			result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}

		if ((usage & BufferUsageFlags::TransferSrc) != BufferUsageFlags::None) {
			result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}

		if ((usage & BufferUsageFlags::TransferDst) != BufferUsageFlags::None) {
			result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}

		if ((usage & BufferUsageFlags::Storage) != BufferUsageFlags::None) {
			result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}

		result |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		return result;
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
	VkFormat toVulkanFormat(Format format, bool sRGB) {
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

	VkImageAspectFlags getAspectMask(Format format) {
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

	VkImageLayout toVulkanLayout(TextureLayout layout) {
		switch (layout) {
		case TextureLayout::Undefined:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case TextureLayout::RenderTarget:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case TextureLayout::DepthStencil:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case TextureLayout::ShaderResource:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case TextureLayout::UnorderedAccess:
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

	VkPipelineStageFlags2 getPipelineStageFlags(ResourceState state) {
		switch (state) {
		case ResourceState::Common:
			return VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		case ResourceState::VertexBuffer:
			return VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
		case ResourceState::IndexBuffer:
			return VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
		case ResourceState::ConstantBuffer:
			return VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		case ResourceState::UnorderedAccess:
			return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		case ResourceState::TransferSrc:
		case ResourceState::CopyDest:
			return VK_PIPELINE_STAGE_2_COPY_BIT;
		case ResourceState::Present:
			return VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
		default:
			SE::LogWarn("Unknown resource state: {}", static_cast<int>(state));
			return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		}
	}

	VkAccessFlags2 getAccessFlags(ResourceState state) {
		switch (state) {
		case ResourceState::VertexBuffer:
			return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		case ResourceState::IndexBuffer:
			return VK_ACCESS_2_INDEX_READ_BIT;
		case ResourceState::ConstantBuffer:
			return VK_ACCESS_2_UNIFORM_READ_BIT;
		case ResourceState::UnorderedAccess:
			return VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		case ResourceState::TransferSrc:
			return VK_ACCESS_2_TRANSFER_READ_BIT;
		case ResourceState::CopyDest:
			return VK_ACCESS_2_TRANSFER_WRITE_BIT;
		default:
			return VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
		}
	}

	inline VkSamplerCreateInfo samplerCreateInfo(const SamplerDescription& desc) {
		validateSamplerDescription(desc);

		VkSamplerCreateInfo info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		info.magFilter = desc.filterMode == FilterMode::Point ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
		info.minFilter = info.magFilter;
		info.mipmapMode = desc.filterMode == FilterMode::Point ?
			VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;

		info.addressModeU = toVulkanAddressMode(desc.addressU);
		info.addressModeV = toVulkanAddressMode(desc.addressV);
		info.addressModeW = toVulkanAddressMode(desc.addressW);
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

		info.format = toVulkanFormat(desc.format);
		info.extent = { desc.width, desc.height, desc.depth };
		info.mipLevels = desc.mipLevels;
		info.arrayLayers = desc.arraySize;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;

		info.usage = 0;
		if ((desc.usage & TextureUsageFlags::RenderTarget) != TextureUsageFlags::None)
			info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if ((desc.usage & TextureUsageFlags::DepthStencil) != TextureUsageFlags::None)
			info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if ((desc.usage & TextureUsageFlags::UnorderedAccess) != TextureUsageFlags::None)
			info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		if ((desc.usage & TextureUsageFlags::ShaderResource) != TextureUsageFlags::None)
			info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		if ((desc.usage & TextureUsageFlags::TransferSrc) != TextureUsageFlags::None)
			info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if ((desc.usage & TextureUsageFlags::TransferDst) != TextureUsageFlags::None)
			info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (desc.type == TextureType::TextureCube ||
			desc.type == TextureType::TextureCubeArray) {
			info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			SE_ASSERT(desc.arraySize % 6 == 0, "Cube texture array size must be a multiple of 6");
		}

		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		return info;
	}

	inline VkSamplerAddressMode toVulkanAddressMode(AddressMode mode) {
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
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	inline VkFilter toVulkanFilter(FilterMode filter) {
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

	inline VkImageViewCreateInfo imageViewCreateInfo() {
		VkImageViewCreateInfo info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		return info;
	}

	inline VkImageViewType getViewType(TextureType type) {
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

	inline VkImageSubresourceRange getSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel,
		uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
		VkImageSubresourceRange range{};
		range.aspectMask = aspectMask;
		range.baseMipLevel = baseMipLevel;
		range.levelCount = levelCount;
		range.baseArrayLayer = baseArrayLayer;
		range.layerCount = layerCount;
		return range;
	}

	inline VkComponentMapping getComponentMapping(bool swizzleRGB) {
		if (swizzleRGB) {
			return { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G,
					VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
		}
		return { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	}

	//struct QueueFamilyIndices {
	//	uint32_t graphics = UINT32_MAX;
	//	uint32_t compute = UINT32_MAX;
	//	uint32_t transfer = UINT32_MAX;
	//	uint32_t present = UINT32_MAX;

	//	bool isComplete() const {
	//		return graphics != UINT32_MAX &&
	//			compute != UINT32_MAX &&
	//			transfer != UINT32_MAX;
	//	}

	//	bool supportsPresent() const {
	//		return present != UINT32_MAX;
	//	}

	//	std::vector<uint32_t> getUniqueIndices() const {
	//		std::vector<uint32_t> indices;
	//		if (graphics != UINT32_MAX) indices.push_back(graphics);

	//		if (compute != UINT32_MAX && compute != graphics)
	//			indices.push_back(compute);

	//		if (transfer != UINT32_MAX && transfer != graphics && transfer != compute)
	//			indices.push_back(transfer);

	//		return indices;
	//	}
	//};

	//inline VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex,
	//	VkCommandPoolCreateFlags flags = 0) {
	//	VkCommandPoolCreateInfo info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	//	info.queueFamilyIndex = queueFamilyIndex;
	//	info.flags = flags;
	//	return info;
	//}

	//inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool,
	//	uint32_t count = 1,
	//	VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
	//	VkCommandBufferAllocateInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	//	info.commandPool = pool;
	//	info.commandBufferCount = count;
	//	info.level = level;
	//	return info;
	//}

	//inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0) {
	//	VkFenceCreateInfo info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	//	info.flags = flags;
	//	return info;
	//}

	//inline VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0) {
	//	VkSemaphoreCreateInfo info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	//	info.flags = flags;
	//	return info;
	//}

	//inline VkSubmitInfo2 submitInfo2() {
	//	VkSubmitInfo2 info{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
	//	return info;
	//}

	//inline VkPresentInfoKHR presentInfo() {
	//	VkPresentInfoKHR info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	//	return info;
	//}

	//inline VkRenderingAttachmentInfo renderingAttachmentInfo() {
	//	VkRenderingAttachmentInfo info{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	//	return info;
	//}

	//inline VkRenderingInfo renderingInfo(VkRect2D renderArea,
	//	uint32_t colorAttachmentCount = 1,
	//	uint32_t layerCount = 1) {
	//	VkRenderingInfo info{ VK_STRUCTURE_TYPE_RENDERING_INFO };
	//	info.renderArea = renderArea;
	//	info.layerCount = layerCount;
	//	info.colorAttachmentCount = colorAttachmentCount;
	//	return info;
	//}
}