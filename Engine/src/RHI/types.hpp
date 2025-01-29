#pragma once

#include<string>
#include<vector>
#include <cfloat>
#include <cstdint>
namespace SE
{
	static const uint32_t SE_MAX_FRAMES_IN_FLIGHT = 3;//For anyone
}
namespace rhi {
	class IShader;
	class ITexture;
	class IHeap;

	// Constants and Static Values
	static const uint32_t SE_MAX_RESOURCE_DESCRIPTOR_COUNT = 65536;
	static const uint32_t SE_MAX_SAMPLER_DESCRIPTOR_COUNT = 64;
	static const uint32_t SE_MAX_UBV_BINDINGS = 3; //push constants in slot 0
	static const uint32_t SE_MAX_PUSH_CONSTANTS = 8;
	static const uint32_t RHI_ALL_SUB_RESOURCE = 0xFFFFFFFF;

	// Enums
	enum class RenderBackend {
		Vulkan,
		D3D12
	};

	enum class PipelineType {
		Compute,
		Graphics,
		Mesh
	};

	enum class RenderPassLoadOp {
		Load,
		Clear,
		DontCare,
	};

	enum class RenderPassStoreOp {
		Store,
		DontCare,
	};

	enum class ShaderType {
		Amplification,
		Mesh,
		Vertex,
		Pixel,
		Compute,
	};

	enum class CullMode {
		None,
		Front,
		Back,
	};

	enum class CompareFunction {
		Never,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always,
	};

	enum class StencilOperation {
		Keep,
		Zero,
		Replace,
		IncrementClamp,
		DecrementClamp,
		Invert,
		IncrementWrap,
		DecrementWrap,
	};

	enum class BlendFactor {
		Zero,
		One,
		SrcColor,
		InvSrcColor,
		SrcAlpha,
		InvSrcAlpha,
		DstAlpha,
		InvDstAlpha,
		DstColor,
		InvDstColor,
		SrcAlphaClamp,
		ConstantFactor,
		InvConstantFactor
	};

	enum class BlendOperation {
		Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max,
	};

	enum class PrimitiveType {
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
	};

	enum class Format {
		Unknown,
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
		R5G6B5_UNORM_PACK16,
		B5G6R5_UNORM_PACK16,
		R5G5B5A1_UNORM_PACK16,
		B5G5R5A1_UNORM_PACK16,
		A1R5G5B5_UNORM_PACK16,
		R4G4_UNORM_PACK8,
		R4G4B4A4_UNORM_PACK16,
		B4G4R4A4_UNORM_PACK16,
		D16_UNORM,
		D24_UNORM_S8_UINT,
		D32_SFLOAT,
		D32_SFLOAT_S8_UINT,
		S8_UINT,
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
		GpuOnly, // Device local only
		CpuOnly, // Staging buffers
		CpuToGpu, // Upload heap
		GpuToCpu // Readback heap
	};

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
		ShaderStorage,
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

	// Structs
	struct RenderPassColorAttachment {
		ITexture* texture = nullptr;
		uint32_t mipSlice = 0;
		uint32_t arraySlice = 0;
		RenderPassLoadOp loadOp = RenderPassLoadOp::Load;
		RenderPassStoreOp storeOp = RenderPassStoreOp::Store;
		float clearColor[4] = {};
	};

	struct RenderPassDepthAttachment {
		ITexture* texture = nullptr;
		uint32_t mipSlice = 0;
		uint32_t arraySlice = 0;
		RenderPassLoadOp loadOp = RenderPassLoadOp::Load;
		RenderPassStoreOp storeOp = RenderPassStoreOp::Store;
		RenderPassLoadOp stencilLoadOp = RenderPassLoadOp::Load;
		RenderPassStoreOp stencilStoreOp = RenderPassStoreOp::Store;
		float clearDepth = 0.0f;
		uint32_t clearStencil = 0;
		bool readOnly = false;
	};

	struct RenderPassDescription {
		RenderPassColorAttachment color[8];
		RenderPassDepthAttachment depth;
	};

	struct Rasterizer
	{
		CullMode cullMode = CullMode::None;
		float depthBias = 0.0f;
		float depthBiasClamp = 0.0f;
		float slopeScaledDepthBias = 0.0f;
		bool wireframe = false;
		bool frontCounterClockwise = false;
		bool depthClip = true;
		bool lineAntialiasing = false;
		bool conservativeRaster = false;
	};

	struct DepthStencilOperation {
		StencilOperation stencilFail = StencilOperation::Keep;
		StencilOperation depthFail = StencilOperation::Keep;
		StencilOperation pass = StencilOperation::Keep;
		CompareFunction stencilFunction = CompareFunction::Always;
	};

	struct DepthStencil {
		CompareFunction depthFunction = CompareFunction::Always;
		bool depthTest = false;
		bool depthWrite = true;
		DepthStencilOperation frontFace;
		DepthStencilOperation backFace;
		bool stencilTest = false;
		uint8_t stencilReadMask = 0xFF;
		uint8_t stencilWriteMask = 0xFF;
	};

	enum class ColorWriteMask : uint32_t {
		Red = 1 << 0,
		Green = 1 << 1,
		Blue = 1 << 2,
		Alpha = 1 << 3,
		All = Red | Green | Blue | Alpha
	};
	struct Blend {
		bool blendEnabled = false;
		BlendFactor colorSource = BlendFactor::One;
		BlendFactor colorDestination = BlendFactor::One;
		BlendOperation colorOperation = BlendOperation::Add;
		BlendFactor alphaSource = BlendFactor::One;
		BlendFactor alphaDestination = BlendFactor::One;
		BlendOperation alphaOperation = BlendOperation::Add;
		ColorWriteMask writeMask = ColorWriteMask::All;
	};

	struct GraphicsPipelineDescription {
		IShader* vertexShader = nullptr;
		IShader* pixelShader = nullptr;
		Rasterizer rasterizer;
		DepthStencil depthStencil;
		Blend blend[8];
		Format renderTargetFormat[8] = { Format::Unknown };
		Format depthStencilFormat = Format::Unknown;
		PrimitiveType primitiveType = PrimitiveType::TriangleList;
	};

	struct ComputePipelineDescription {
		IShader* computeShader = nullptr;
	};

	enum class BufferUsageFlags : uint32_t {
		None = 0,
		StructuredBuffer = 1 << 0,
		FormattedBuffer = 1 << 1,
		UniformBuffer = 1 << 2,
		RawBuffer = 1 << 3,
		ShaderStorageBuffer = 1 << 4,
	};

	inline BufferUsageFlags operator|(BufferUsageFlags a, BufferUsageFlags b) {
		return static_cast<BufferUsageFlags>(static_cast<int>(a) | static_cast<int>(b));
	}

	inline BufferUsageFlags operator&(BufferUsageFlags a, BufferUsageFlags b) {
		return static_cast<BufferUsageFlags>(static_cast<int>(a) & static_cast<int>(b));
	}

	inline BufferUsageFlags& operator|=(BufferUsageFlags& a, BufferUsageFlags b) {
		a = a | b;
		return a;
	}

	enum class TextureUsageFlags : uint32_t {
		None = 0,
		RenderTarget = 1 << 0,
		DepthStencil = 1 << 1,
		ShaderStorage = 1 << 2,
		Shared = 1 << 3
	};

	inline TextureUsageFlags operator|(TextureUsageFlags a, TextureUsageFlags b) {
		return static_cast<TextureUsageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline TextureUsageFlags operator&(TextureUsageFlags a, TextureUsageFlags b) {
		return static_cast<TextureUsageFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	inline TextureUsageFlags& operator|=(TextureUsageFlags& a, TextureUsageFlags b) {
		a = a | b;
		return a;
	}

	enum class ResourceAccessFlags : uint32_t {
		None = 0,
		Present = 1 << 0,
		RenderTarget = 1 << 1,
		DepthStencilStorage = 1 << 2,
		DepthStencilRead = 1 << 3,
		VertexShaderRead = 1 << 4,
		PixelShaderRead = 1 << 5,
		ComputeShaderRead = 1 << 6,
		VertexShaderStorage = 1 << 7,
		PixelShaderStorage = 1 << 8,
		ComputeShaderStorage = 1 << 9,
		StorageClear = 1 << 10,
		TransferDst = 1 << 11,
		TransferSrc = 1 << 12,
		ShadingRate = 1 << 13,
		IndexBuffer = 1 << 14,
		IndirectArgs = 1 << 15,
		AccelerationStructureRead = 1 << 16,
		AccelerationStructureStorage = 1 << 17,
		Discard = 1 << 18, // Aliasing barrier

		// Composite flags
		MaskShaderVS = VertexShaderRead | VertexShaderStorage,
		MaskShaderPS = PixelShaderRead | PixelShaderStorage,
		MaskShaderCS = ComputeShaderRead | ComputeShaderStorage,
		MaskShaderRead = VertexShaderRead | PixelShaderRead | ComputeShaderRead,
		MaskShaderStorage = VertexShaderStorage | PixelShaderStorage | ComputeShaderStorage,
		MaskDepthStencilAccess = DepthStencilStorage | DepthStencilRead,
		MaskTransferAccess = TransferDst | TransferSrc,
		MaskAccelerationStructureAccess = AccelerationStructureRead | AccelerationStructureStorage
	};

	inline ResourceAccessFlags operator|(ResourceAccessFlags lhs, ResourceAccessFlags rhs) {
		return static_cast<ResourceAccessFlags>(
			static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
			);
	};

	inline ResourceAccessFlags operator&(ResourceAccessFlags lhs, ResourceAccessFlags rhs) {
		return static_cast<ResourceAccessFlags>(
			static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)
			);
	};

	inline ResourceAccessFlags& operator|=(ResourceAccessFlags& lhs, ResourceAccessFlags rhs) {
		lhs = lhs | rhs;
		return lhs;
	};

	enum class ShaderResourceViewDescriptorType {
		Texture2D,
		Texture2DArray,
		Texture3D,
		TextureCube,
		TextureCubeArray,
		StructuredBuffer,
		FormattedBuffer,
		RawBuffer,
		AccelerationStructure
	};
	enum class UnorderedAccessDescriptorType
	{
		Texture2D,
		Texture2DArray,
		Texture3D,
		StructuredBuffer,
		FormattedBuffer,
		RawBuffer
	};

	struct ConstantBufferDescriptorDescription
	{
		uint32_t size = 0;
		uint32_t offset = 0;
	};

	struct ShaderResourceViewDescriptorDescription
	{
		ShaderResourceViewDescriptorType type = ShaderResourceViewDescriptorType::Texture2D;
		Format format = Format::Unknown;

		union
		{
			struct
			{
				uint32_t mipSlice = 0;
				uint32_t arraySlice = 0;
				uint32_t mipLevels = uint32_t(-1);
				uint32_t arraySize = 1;
				uint32_t planeSlice = 0;
			} texture;

			struct
			{
				uint32_t size = 0;
				uint32_t offset = 0;
			} buffer;
		};

		ShaderResourceViewDescriptorDescription() : texture() {}
	};

	inline bool operator==(const ShaderResourceViewDescriptorDescription& lhs, const ShaderResourceViewDescriptorDescription& rhs)
	{
		return lhs.type == rhs.type &&
			lhs.texture.mipSlice == rhs.texture.mipSlice &&
			lhs.texture.mipLevels == rhs.texture.mipLevels &&
			lhs.texture.arraySlice == rhs.texture.arraySlice &&
			lhs.texture.arraySize == rhs.texture.arraySize &&
			lhs.texture.planeSlice == rhs.texture.planeSlice;
	}

	struct UnorderedAccessDescriptorDescription
	{
		UnorderedAccessDescriptorType type = UnorderedAccessDescriptorType::Texture2D;
		Format format = Format::Unknown;

		union
		{
			struct
			{
				uint32_t mipSlice = 0;
				uint32_t arraySlice = 0;
				uint32_t arraySize = 1;
				uint32_t planeSlice = 0;
			} texture;

			struct
			{
				uint32_t size = 0;
				uint32_t offset = 0;
			} buffer;
		};

		UnorderedAccessDescriptorDescription() : texture() {}
	};

	inline bool operator==(const UnorderedAccessDescriptorDescription& lhs, const UnorderedAccessDescriptorDescription& rhs)
	{
		return lhs.type == rhs.type &&
			lhs.texture.mipSlice == rhs.texture.mipSlice &&
			lhs.texture.arraySlice == rhs.texture.arraySlice &&
			lhs.texture.arraySize == rhs.texture.arraySize &&
			lhs.texture.planeSlice == rhs.texture.planeSlice;
	}

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

	struct BufferDescription {
		uint64_t size = 0;
		uint32_t stride = 0;
		Format format = Format::Unknown;
		MemoryType memoryType = MemoryType::GpuOnly;
		BufferUsageFlags usage = BufferUsageFlags::None;
		bool mapped = false;           // Whether buffer should be persistently mapped
		IHeap* heap = nullptr;
		uint32_t heapOffset = 0;
		bool operator==(const BufferDescription& other) const {
			return size == other.size &&
				stride == other.stride &&
				memoryType == other.memoryType &&
				usage == other.usage &&
				mapped == other.mapped &&
				heap == other.heap &&
				heapOffset == other.heapOffset;
		}

		bool operator!=(const BufferDescription& other) const {
			return !(*this == other);
		}
	};

	struct TextureDescription {
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
		uint32_t mipLevels = 1;
		uint32_t arraySize = 1;
		Format format = Format::Unknown;
		TextureType type = TextureType::Texture2D;
		TextureUsageFlags usage = TextureUsageFlags::None;
		MemoryType memoryType = MemoryType::GpuOnly;
		IHeap* heap = nullptr;
		uint32_t heapOffset = 0;
		bool operator==(const TextureDescription& other) const {
			return width == other.width &&
				height == other.height &&
				depth == other.depth &&
				mipLevels == other.mipLevels &&
				arraySize == other.arraySize &&
				format == other.format &&
				type == other.type &&
				usage == other.usage &&
				memoryType == other.memoryType &&
				heap == other.heap &&
				heapOffset == other.heapOffset;
		}

		bool operator!=(const TextureDescription& other) const {
			return !(*this == other);
		}
	};

	//bool operator==(const BufferDescription& lhs, const BufferDescription& rhs) {
	//	return (lhs.size == rhs.size) &&
	//		(lhs.stride == rhs.stride) &&
	//		(lhs.memoryType == rhs.memoryType) &&
	//		(lhs.usage == rhs.usage) &&
	//		(lhs.mapped == rhs.mapped) &&
	//		(lhs.heap == rhs.heap) &&
	//		(lhs.heapOffset == rhs.heapOffset);
	//}

	//bool operator!=(const BufferDescription& lhs, const BufferDescription& rhs) {
	//	return !(lhs == rhs);
	//}

	//bool operator==(const TextureDescription& lhs, const TextureDescription& rhs) {
	//	return (lhs.width == rhs.width) &&
	//		(lhs.height == rhs.height) &&
	//		(lhs.depth == rhs.depth) &&
	//		(lhs.mipLevels == rhs.mipLevels) &&
	//		(lhs.arraySize == rhs.arraySize) &&
	//		(lhs.format == rhs.format) &&
	//		(lhs.type == rhs.type) &&
	//		(lhs.usage == rhs.usage) &&
	//		(lhs.memoryType == rhs.memoryType) &&
	//		(lhs.heap == rhs.heap) &&         // pointer comparison (address equality)
	//		(lhs.heapOffset == rhs.heapOffset);
	//}

	//bool operator!=(const TextureDescription& lhs, const TextureDescription& rhs) {
	//	return !(lhs == rhs);
	//}

	struct DeviceDescription {
		void* windowHandle = nullptr;
		bool enableValidation = true;
		RenderBackend backend = RenderBackend::Vulkan;
	};
	struct ShaderDescription
	{
		ShaderType type;
		std::string file;
		std::string entryPoint;
		std::vector<std::string> defines;
	};
	struct SwapchainDescription {
		void* windowHandle = nullptr;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t bufferCount = 3;
		Format format = Format::Unknown;
		bool vsync = true;
	};

	struct HeapDescription
	{
		uint32_t size = 1;
		MemoryType memoryType = MemoryType::GpuOnly;
	};

	template<typename Enum>
	inline bool anySet(Enum flags, Enum mask) {
		using underlying = typename std::underlying_type<Enum>::type;
		return (static_cast<underlying>(flags) & static_cast<underlying>(mask)) != 0;
	}
}