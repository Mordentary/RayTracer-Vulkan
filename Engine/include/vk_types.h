#pragma once
#include"engine_core.h"

#include <vk_mem_alloc.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <vector>

namespace Engine
{
	// Allocated image structure
	struct AllocatedImage {
		VkImage image;
		VkImageView imageView;
		VmaAllocation allocation;
		VkExtent3D extent;
		VkFormat format;
	};

	// Allocated buffer structure
	struct AllocatedBuffer {
		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
	};

	// GPU GLTF material structure
	struct GPUGLTFMaterial {
		glm::vec4 colorFactors;
		glm::vec4 metalRoughFactors;
		glm::vec4 padding[14];
	};

	static_assert(sizeof(GPUGLTFMaterial) == 256, "GPUGLTFMaterial size must be 256 bytes");

	// GPU scene data structure
	struct GPUSceneData {
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 viewProjection;
		glm::vec4 ambientColor;
		glm::vec4 sunlightDirection; // w component for sun power
		glm::vec4 sunlightColor;
	};

	// Material pass enumeration
	enum class MaterialPass : uint8_t {
		MainColor,
		Transparent,
		Other
	};

	// Material pipeline structure
	struct MaterialPipeline {
		VkPipeline pipeline;
		VkPipelineLayout layout;
	};

	// Material instance structure
	struct MaterialInstance {
		MaterialPipeline* pipeline;
		VkDescriptorSet descriptorSet;
		MaterialPass passType;
	};

	// Vertex structure aligned to 16 bytes
	struct alignas(16) Vertex {
		glm::vec3 position;
		float uvX;
		glm::vec3 normal;
		float uvY;
		glm::vec4 color;
	};

	// GPU mesh buffers structure
	struct GPUMeshBuffers {
		AllocatedBuffer indexBuffer;
		AllocatedBuffer vertexBuffer;
		VkDeviceAddress vertexBufferAddress;
	};

	// GPU draw push constants structure
	struct GPUDrawPushConstants {
		glm::mat4 worldMatrix;
		VkDeviceAddress vertexBufferAddress;
	};

	// Drawing context forward declaration
	struct DrawingContext;

	// Renderable interface
	class IRenderable {
	public:
		virtual void draw(const glm::mat4& parentTransform, DrawingContext& context) = 0;
	};
	// Scene node structure inheriting from IRenderable
	struct SceneNode : public IRenderable
	{
		std::weak_ptr<SceneNode> parent;
		std::vector<Shared<SceneNode>> children{};
		glm::mat4 localTransform{};
		glm::mat4 worldTransform{};

		void refreshTransform(const glm::mat4& parentMatrix)
		{
			worldTransform = parentMatrix * localTransform;
			for (const auto& child : children) {
				child->refreshTransform(worldTransform);
			}
		}

		virtual void draw(const glm::mat4& parentTransform, DrawingContext& context) override
		{
			// draw children
			for (const auto& child : children) {
				child->draw(parentTransform, context);
			}
		}
	};
}
