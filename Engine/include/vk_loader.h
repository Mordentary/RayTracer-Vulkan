#pragma once
#include "vk_types.h" 
#include "vk_descriptors.h"

#include <unordered_map>
#include <filesystem>
#include <fastgltf/types.hpp>



namespace SE
{
	class Engine;
	struct DescriptorAllocator;

	struct GLTFMaterial
	{
		MaterialInstance data;
	};
	struct GeoSurface
	{
		uint32_t startIndex;
		uint32_t count;
		Shared<GLTFMaterial> material;
	};

	struct MeshAsset
	{
		std::string name;

		std::vector<GeoSurface> surfaces;
		GPUMeshBuffers meshBuffers;
	};

	struct LoadedGLTF : public IRenderable {

		// storage for all the data on a given glTF file
		std::unordered_map<std::string, Shared<MeshAsset>> meshes;
		std::unordered_map<std::string, Shared<SceneNode>> nodes;
		std::unordered_map<std::string, AllocatedImage> images;
		std::unordered_map<std::string, Shared<GLTFMaterial>> materials;

		// nodes that dont have a parent, for iterating through the file in tree order
		std::vector<Shared<SceneNode>> topNodes;
		std::vector<VkSampler> samplers;
		DescriptorAllocator descriptorPool;
		AllocatedBuffer materialDataBuffer;
		Engine* creator;

		~LoadedGLTF() { clearAll(); };

		virtual void draw(const glm::mat4& topMatrix, DrawingContext& ctx);

	private:
		void clearAll();
	};
	std::optional<Shared<LoadedGLTF>> loadGltfMeshes(Engine* engine, std::filesystem::path filePath);
	std::optional<AllocatedImage> loadImage(Engine* engine, fastgltf::Asset& asset, fastgltf::Image& image);
}
