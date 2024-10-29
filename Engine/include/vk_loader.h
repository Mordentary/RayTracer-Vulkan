#pragma once

#include "vk_types.h"
#include "vk_descriptors.h"

#include <unordered_map>
#include <filesystem>
#include <fastgltf/types.hpp>

namespace SE {
	class Engine;
	struct DescriptorAllocator;

	// Represents a surface within a mesh, associated with a material
	struct Surface {
		uint32_t startIndex{ 0 };
		uint32_t indexCount{ 0 };
		Shared<MaterialInstance> material;
	};

	class MeshAsset {
	public:
		MeshAsset() = default;

		//THE OWNER FREE OBJECT's resources
		~MeshAsset() = default;

		void cleanup(Engine* engine);
		void uploadToGPU(Engine* engine,
			const std::span<uint32_t>& indices,
			const std::span<Vertex>& vertices);

		// Accessors
		const std::string& getName() const { return name; }
		void setName(const std::string& newName) { name = newName; }

		const std::vector<Surface>& getSurfaces() const { return surfaces; }
		void addSurface(const Surface& surface) { surfaces.push_back(surface); }

		const GPUMeshBuffers& getMeshBuffers() const { return meshBuffers; }

	private:
		std::string name;
		std::vector<Surface> surfaces;
		GPUMeshBuffers meshBuffers;
	};

	// Loader class for materials in GLTF files
	class GLTFMaterialLoader {
	public:
		struct LoadContext {
			Engine* engine;
			const fastgltf::Asset& gltf;
			fastgltf::Material* material;
			size_t materialIndex;
			AllocatedBuffer& constantBuffer;
			const MaterialFactory::Resources& defaults;
			const std::vector<AllocatedImage>& images;
			const std::vector<VkSampler>& samplers;
		};

		GLTFMaterialLoader() = delete;  // Static class

		static MaterialInstance loadMaterial(const LoadContext& ctx);

	private:
		static MaterialFactory::Constants extractConstants(const fastgltf::Material& material);
		static MaterialFactory::Resources createResources(const LoadContext& ctx);
		static MaterialPass determinePassType(const fastgltf::Material& material);

		// Helper for texture loading
		static std::optional<std::pair<AllocatedImage, VkSampler>> tryGetTexture(
			const fastgltf::TextureInfo* textureInfo, const LoadContext& ctx);

		static glm::vec4 toVec4(const std::array<float, 4>& arr);
	};
	class GLTFLoader;
	// GLTF Loader class for loading GLTF files into LoadedGLTF instances

	// Represents a loaded GLTF scene, implementing IRenderable
	class LoadedGLTF : public IRenderable {
		friend GLTFLoader;
	public:
		explicit LoadedGLTF(Engine* engine);
		~LoadedGLTF();

		void draw(const glm::mat4& topMatrix, RenderQueue& renderQueue) override;

		// Accessors
		const std::unordered_map<std::string, Shared<MeshAsset>>& getMeshes() const { return meshes; }
		const std::unordered_map<std::string, Shared<SceneNode>>& getNodes() const { return nodes; }
		const std::unordered_map<std::string, AllocatedImage>& getImages() const { return images; }
		const std::unordered_map<std::string, Shared<MaterialInstance>>& getMaterials() const { return materials; }
		const std::vector<Shared<SceneNode>>& getTopNodes() const { return topNodes; }

	private:
		void clearAll();

		// Data storage
		std::unordered_map<std::string, Shared<MeshAsset>> meshes;
		std::unordered_map<std::string, Shared<SceneNode>> nodes;
		std::vector<Shared<SceneNode>> topNodes;
		std::unordered_map<std::string, AllocatedImage> images;
		std::unordered_map<std::string, Shared<MaterialInstance>> materials;

		std::vector<VkSampler> samplers;
		AllocatedBuffer materialDataBuffer;
		Engine* creator;
	};

	class GLTFLoader {
	public:
		static std::optional<Shared<LoadedGLTF>> loadGLTF(Engine* engine, const std::filesystem::path& filePath);

	private:
		static std::optional<AllocatedImage> loadImage(Engine* engine, fastgltf::Asset& asset, fastgltf::Image& image);
	};
}  // namespace SE
