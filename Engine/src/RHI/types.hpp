#pragma once

namespace rhi {
	static const uint32_t SE_MAX_FRAMES_IN_FLIGHT = 3;
	static const uint32_t SE_MAX_RESOURCE_DESCRIPTOR_COUNT = 65536;
	static const uint32_t SE_MAX_SAMPLER_DESCRIPTOR_COUNT = 64;

	enum class RenderBackend
	{
		Vulkan,
		D3D12
	};

	enum class ResourceState {
		Undefined,
		Common,
		VertexBuffer,
		IndexBuffer,
		ConstantBuffer,
		UnorderedAccess,
		CopySource,
		CopyDest,
		Present
	};

	enum class Format {
		Unknown,
		R8G8B8A8_UNORM,
		R8G8B8A8_SRGB,
		B8G8R8A8_UNORM,
		B8G8R8A8_SRGB,
		R16G16B16A16_SFLOAT,
		R32G32B32A32_SFLOAT,
		D32_SFLOAT
	};

	enum class CommandType {
		Graphics,
		Compute,
		Copy
	};
	enum class MemoryType {
		GpuOnly,        // Device local only
		CpuOnly,		//Staging buffers
		CpuToGpu,      // Upload heap
		GpuToCpu,      // Readback heap
	};

	enum class BufferUsageFlags {
		None = 0,
		VertexBuffer = 1 << 0,
		IndexBuffer = 1 << 1,
		ConstantBuffer = 1 << 2,
		UnorderedAccess = 1 << 3,
		TransferSrc = 1 << 4,
		TransferDst = 1 << 5,
		Storage = 1 << 6,
	};

	inline BufferUsageFlags operator|(BufferUsageFlags a, BufferUsageFlags b) {
		return static_cast<BufferUsageFlags>(static_cast<int>(a) | static_cast<int>(b));
	}

	inline BufferUsageFlags operator&(BufferUsageFlags a, BufferUsageFlags b) {
		return static_cast<BufferUsageFlags>(static_cast<int>(a) & static_cast<int>(b));
	}
} // namespace rhi