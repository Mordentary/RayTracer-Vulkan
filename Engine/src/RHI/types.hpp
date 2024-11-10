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
		TransferSrc,
		CopyDest,
		Present
	};

	enum class Format {
		Unknown,

		// 8-bit formats
		R8_UNORM,
		R8_SNORM,
		R8_UINT,
		R8_SINT,
		R8G8_UNORM,
		R8G8_SNORM,
		R8G8_UINT,
		R8G8_SINT,
		R8G8B8_UNORM,
		R8G8B8_SNORM,
		R8G8B8_UINT,
		R8G8B8_SINT,
		R8G8B8A8_UNORM,
		R8G8B8A8_SNORM,
		R8G8B8A8_UINT,
		R8G8B8A8_SINT,
		R8G8B8A8_SRGB,
		B8G8R8A8_UNORM,
		B8G8R8A8_SNORM,
		B8G8R8A8_UINT,
		B8G8R8A8_SINT,
		B8G8R8A8_SRGB,
		// 16-bit formats
		R16_UNORM,
		R16_SNORM,
		R16_UINT,
		R16_SINT,
		R16_SFLOAT,
		R16G16_UNORM,
		R16G16_SNORM,
		R16G16_UINT,
		R16G16_SINT,
		R16G16_SFLOAT,
		R16G16B16_UNORM,
		R16G16B16_SNORM,
		R16G16B16_UINT,
		R16G16B16_SINT,
		R16G16B16_SFLOAT,
		R16G16B16A16_UNORM,
		R16G16B16A16_SNORM,
		R16G16B16A16_UINT,
		R16G16B16A16_SINT,
		R16G16B16A16_SFLOAT,

		// 32-bit formats
		R32_UINT,
		R32_SINT,
		R32_SFLOAT,
		R32G32_UINT,
		R32G32_SINT,
		R32G32_SFLOAT,
		R32G32B32_UINT,
		R32G32B32_SINT,
		R32G32B32_SFLOAT,
		R32G32B32A32_UINT,
		R32G32B32A32_SINT,
		R32G32B32A32_SFLOAT,

		// Packed formats
		R5G6B5_UNORM_PACK16,
		B5G6R5_UNORM_PACK16,
		R5G5B5A1_UNORM_PACK16,
		B5G5R5A1_UNORM_PACK16,
		A1R5G5B5_UNORM_PACK16,
		R4G4_UNORM_PACK8,
		R4G4B4A4_UNORM_PACK16,
		B4G4R4A4_UNORM_PACK16,

		// Depth-stencil formats
		D16_UNORM,
		D24_UNORM_S8_UINT,
		D32_SFLOAT,
		D32_SFLOAT_S8_UINT,
		S8_UINT,

		// Compressed formats
		BC1_UNORM,
		BC1_SRGB,
		BC2_UNORM,
		BC2_SRGB,
		BC3_UNORM,
		BC3_SRGB,
		BC4_UNORM,
		BC4_SNORM,
		BC5_UNORM,
		BC5_SNORM,
		BC6H_UFLOAT,
		BC6H_SFLOAT,
		BC7_UNORM,
		BC7_SRGB,

		// ASTC (Adaptive Scalable Texture Compression)
		ASTC_4x4_UNORM,
		ASTC_4x4_SRGB,
		ASTC_5x5_UNORM,
		ASTC_5x5_SRGB,
		ASTC_6x6_UNORM,
		ASTC_6x6_SRGB,
		ASTC_8x8_UNORM,
		ASTC_8x8_SRGB,
		ASTC_10x10_UNORM,
		ASTC_10x10_SRGB,
		ASTC_12x12_UNORM,
		ASTC_12x12_SRGB,

		// ETC (Ericsson Texture Compression)
		ETC2_R8G8B8_UNORM,
		ETC2_R8G8B8_SRGB,
		ETC2_R8G8B8A1_UNORM,
		ETC2_R8G8B8A1_SRGB,
		ETC2_R8G8B8A8_UNORM,
		ETC2_R8G8B8A8_SRGB,
		EAC_R11_UNORM,
		EAC_R11_SNORM,
		EAC_R11G11_UNORM,
		EAC_R11G11_SNORM,

		// YUV formats
		YUV420_8BIT,
		YUV420_10BIT,
		YUV422_8BIT,
		YUV422_10BIT,
		YUV444_8BIT,
		YUV444_10BIT
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

	enum class BufferUsageFlags : uint32_t {
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
	enum class TextureUsageFlags : uint32_t {
		None = 0,
		RenderTarget = 1 << 0,
		DepthStencil = 1 << 1,
		UnorderedAccess = 1 << 2,
		ShaderResource = 1 << 3,
		TransferSrc = 1 << 4,
		TransferDst = 1 << 5,
		Storage = 1 << 6,
		Shared = 1 << 7
	};
	// For TextureUsageFlags
	inline TextureUsageFlags operator|(TextureUsageFlags a, TextureUsageFlags b) {
		return static_cast<TextureUsageFlags>(
			static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline TextureUsageFlags operator&(TextureUsageFlags a, TextureUsageFlags b) {
		return static_cast<TextureUsageFlags>(
			static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	inline TextureUsageFlags& operator|=(TextureUsageFlags& a, TextureUsageFlags b) {
		a = a | b;
		return a;
	}

	enum class ResourceAccessFlags : uint32_t
	{
		None = 0,
		Present = 1 << 0,
		RenderTarget = 1 << 1,
		DepthStencil = 1 << 2,
		DepthStencilReadOnly = 1 << 3,
		VertexShaderSRV = 1 << 4,
		PixelShaderSRV = 1 << 5,
		ComputeShaderSRV = 1 << 6,
		VertexShaderUAV = 1 << 7,
		PixelShaderUAV = 1 << 8,
		ComputeShaderUAV = 1 << 9,
		ClearUAV = 1 << 10,
		TransferDst = 1 << 11,
		TransferSrc = 1 << 12,
		ShadingRate = 1 << 13,
		IndexBuffer = 1 << 14,
		IndirectArgs = 1 << 15,
		AccelerationStructureRead = 1 << 16,
		AccelerationStructureWrite = 1 << 17,
		Discard = 1 << 18, // Aliasing barrier

		VertexShaderAccess = VertexShaderSRV | VertexShaderUAV,
		PixelShaderAccess = PixelShaderSRV | PixelShaderUAV,
		ComputeShaderAccess = ComputeShaderSRV | ComputeShaderUAV,
		ShaderResourceAccess = VertexShaderSRV | PixelShaderSRV | ComputeShaderSRV,
		UnorderedAccess = VertexShaderUAV | PixelShaderUAV | ComputeShaderUAV,
		DepthStencilAccess = DepthStencil | DepthStencilReadOnly,
		CopyAccess = TransferDst | TransferSrc,
		AccelerationStructureAccess = AccelerationStructureRead | AccelerationStructureWrite
	};

	// For ResourceAccessFlags
	inline ResourceAccessFlags operator|(ResourceAccessFlags a, ResourceAccessFlags b) {
		return static_cast<ResourceAccessFlags>(
			static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline ResourceAccessFlags operator&(ResourceAccessFlags a, ResourceAccessFlags b) {
		return static_cast<ResourceAccessFlags>(
			static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	inline ResourceAccessFlags& operator|=(ResourceAccessFlags& a, ResourceAccessFlags b) {
		a = a | b;
		return a;
	}

	enum class TextureType {
		Texture1D,
		Texture2D,
		Texture2DArray,
		Texture3D,
		TextureCube,
		TextureCubeArray
	};

	enum class TextureLayout {
		Undefined,
		General,
		RenderTarget,
		DepthStencil,
		ShaderResource,
		UnorderedAccess,
		TransferSrc,
		TransferDst,
		Present
	};
	enum class FilterMode {
		Point,
		Bilinear,
		Trilinear,
		Anisotropic
	};

	enum class AddressMode {
		Wrap,
		Mirror,
		Clamp,
		Border
	};

	struct SamplerDescription {
		FilterMode filterMode = FilterMode::Point;
		AddressMode addressU = AddressMode::Wrap;
		AddressMode addressV = AddressMode::Wrap;
		AddressMode addressW = AddressMode::Wrap;
		float mipLodBias = 0.0f;
		float maxAnisotropy = 1.0f;
		float minLod = 0.0f;
		float maxLod = FLT_MAX;
	};
} // namespace rhi