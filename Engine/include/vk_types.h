#pragma once

#include"engine_core.h"

#include <vk_mem_alloc.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <vector>
#include <fastgltf\types.hpp>

namespace SE
{
	class Engine;
	// Allocated image structure
	struct AllocatedImage {
		VkImage image;
		VkImageView imageView;
		VmaAllocation allocation;
		VkExtent3D extent;
		VkFormat format;
		VkImageLayout currentLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	};

	// Allocated buffer structure
	struct AllocatedBuffer {
		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;

		template<typename T>
		[[nodiscard]] T* getMappedData() const {
			return static_cast<T*>(allocationInfo.pMappedData);
		}
	};

	// GPU scene data structure
	struct GPUSceneData {
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 viewProjection;
		glm::vec4 ambientColor;
		glm::vec4 sunlightDirection; // w component for sun power
		glm::vec4 sunlightColor;
	};

	// Defines the type of material pass for rendering
	enum class MaterialPass : uint8_t {
		MainColor,    // Main opaque pass
		Transparent,  // Transparent objects pass
		Other         // Reserved for future passes
	};

	// Represents a compiled graphics pipeline with its layout
	struct MaterialPipeline {
		VkPipeline pipeline{ VK_NULL_HANDLE };
		VkPipelineLayout layout{ VK_NULL_HANDLE };
	};

	struct MaterialInstance {
		const MaterialPipeline* pipeline{ nullptr };
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
		MaterialPass passType{ MaterialPass::MainColor };
	};

	class MaterialFactory {
	public:
		struct Constants {
			glm::vec4 baseColorFactor{ 1.0f };
			glm::vec4 metallicRoughnessFactors{ 1.0f };
			glm::vec4 emissiveFactor{ 0.0f };
			float alphaCutoff{ 0.5f };
			float padding[3]{};
		};

		struct Resources {
			AllocatedImage albedoTexture;
			AllocatedImage metallicRoughnessTexture;
			AllocatedImage normalTexture;
			AllocatedImage emissiveTexture;
			AllocatedImage occlusionTexture;
			VkSampler sampler{ VK_NULL_HANDLE };
			VkBuffer uniformBuffer{ VK_NULL_HANDLE };
			uint32_t uniformBufferOffset{ 0 };
		};

		// Prevent instantiation - this is a pure factory class
		MaterialFactory() = delete;

		[[nodiscard]] static MaterialInstance createInstance(
			Engine* engine,
			MaterialPass pass,
			const Resources& resources
		);
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
	struct RenderQueue;

	// Renderable interface
	class IRenderable {
	public:
		virtual ~IRenderable() = default;
		virtual void draw(const glm::mat4& parentTransform, RenderQueue& renderQueue) = 0;
		virtual void update(float deltaTime) {}
	};
	// Scene node structure inheriting from IRenderable
	class SceneNode :
		public IRenderable,
		public std::enable_shared_from_this<SceneNode> {
	public:
		SceneNode() = default;
		virtual ~SceneNode() = default;

		// Prevent copying, allow moving
		SceneNode(const SceneNode&) = delete;
		SceneNode& operator=(const SceneNode&) = delete;
		SceneNode(SceneNode&&) = default;
		SceneNode& operator=(SceneNode&&) = default;

		// Scene graph manipulation
		void addChild(Shared<SceneNode>& child) {
			child->m_Parent = shared_from_this();
			m_Children.push_back((child));
		}

		void removeChild(const SceneNode* child) {
			auto it = std::find_if(m_Children.begin(), m_Children.end(),
				[child](const auto& ptr) { return ptr.get() == child; });
			if (it != m_Children.end()) {
				(*it)->m_Parent.reset();
				m_Children.erase(it);
			}
		}

		// Transform manipulation

		// Getters
		[[nodiscard]] const glm::mat4& getLocalTransform() const { return m_LocalTransform; }
		[[nodiscard]] const glm::mat4& getWorldTransform() const { return m_WorldTransform; }
		[[nodiscard]] const std::vector<Shared<SceneNode>>& getChildren() const { return m_Children; }
		[[nodiscard]] Weak<SceneNode> getParent() const { return m_Parent; }

		virtual void draw(const glm::mat4& topTransform, RenderQueue& renderQueue) override {
			// Calculate world transform
			//m_WorldTransform = topTransform * m_LocalTransform;

			// Draw all children with the updated transform
			for (const auto& child : m_Children) {
				child->draw(topTransform, renderQueue);
			}
		}

		// Update function for animations or other time-based updates
		virtual void update(float deltaTime) override {
			for (const auto& child : m_Children) {
				child->update(deltaTime);
			}
		}

		//void updateWorldTransform() {
		//	if (auto parent = m_Parent.lock())
		//	{
		//		m_WorldTransform = parent->getWorldTransform() * m_LocalTransform;
		//	}
		//	//else
		//	//{
		//	//	m_WorldTransform = m_LocalTransform;
		//	//}
		//	// Propagate transform update to children
		//	for (const auto& child : m_Children) {
		//		child->updateWorldTransform();
		//	}
		//}

		void refreshTransform(const glm::mat4& parentMatrix)
		{
			m_WorldTransform = parentMatrix * m_LocalTransform;
			for (auto c : m_Children) {
				c->refreshTransform(m_WorldTransform);
			}
		}

		void setLocalTransform(const glm::mat4& transform) {
			m_LocalTransform = transform;
			//updateWorldTransform();
		}

	protected:

		void setParent(const std::shared_ptr<SceneNode>& parent) { m_Parent = parent; }

		Weak<SceneNode> m_Parent;
		std::vector<Shared<SceneNode>> m_Children;

		glm::mat4 m_LocalTransform{ 1.0f };
		glm::mat4 m_WorldTransform{ 1.0f };

		// Update world transform and propagate to children
	};
}
