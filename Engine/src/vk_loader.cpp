#include "vk_loader.h"
#include "vk_engine.h"

#include <fastgltf/parser.hpp>
#include <fastgltf/util.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/base64.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <stb_image.h>
#include <glm/gtx/quaternion.hpp>

#include <iostream>
#include <unordered_set>

namespace SE {
	namespace {
		// Helper functions to map GLTF sampler settings to Vulkan equivalents
		VkFilter extractFilter(fastgltf::Filter filter) {
			switch (filter) {
			case fastgltf::Filter::Nearest:
			case fastgltf::Filter::NearestMipMapNearest:
			case fastgltf::Filter::NearestMipMapLinear:
				return VK_FILTER_NEAREST;
			case fastgltf::Filter::Linear:
			case fastgltf::Filter::LinearMipMapNearest:
			case fastgltf::Filter::LinearMipMapLinear:
			default:
				return VK_FILTER_LINEAR;
			}
		}

		VkSamplerMipmapMode extractMipmapMode(fastgltf::Filter filter) {
			switch (filter) {
			case fastgltf::Filter::NearestMipMapNearest:
			case fastgltf::Filter::LinearMipMapNearest:
				return VK_SAMPLER_MIPMAP_MODE_NEAREST;
			case fastgltf::Filter::NearestMipMapLinear:
			case fastgltf::Filter::LinearMipMapLinear:
			default:
				return VK_SAMPLER_MIPMAP_MODE_LINEAR;
			}
		}
	}  // namespace

	// MeshAsset implementation
	//MeshAsset::~MeshAsset() {
	//}

	// Cleans up GPU resources associated with the mesh
	void MeshAsset::cleanup(Engine* engine) {
		if (meshBuffers.indexBuffer.buffer) {
			engine->destroyBuffer(meshBuffers.indexBuffer);
		}
		if (meshBuffers.vertexBuffer.buffer) {
			engine->destroyBuffer(meshBuffers.vertexBuffer);
		}
		surfaces.clear();
	}

	// Uploads mesh data to the GPU
	void MeshAsset::uploadToGPU(Engine* engine,
		const std::span<uint32_t>& indices,
		const std::span<Vertex>& vertices) {
		meshBuffers = engine->uploadMesh(indices, vertices);
	}

	// GLTFMaterialLoader implementation
	MaterialInstance GLTFMaterialLoader::loadMaterial(const LoadContext& ctx) {
		// Write constants to buffer
		auto constants = extractConstants(*ctx.material);
		auto* mappedConstants = ctx.constantBuffer.getMappedData<MaterialFactory::Constants>();
		mappedConstants[ctx.materialIndex] = constants;

		// Create resources
		auto resources = createResources(ctx);

		// Create material instance
		return MaterialFactory::createInstance(
			ctx.engine,
			determinePassType(*ctx.material),
			resources
		);
	}

	MaterialFactory::Constants GLTFMaterialLoader::extractConstants(const fastgltf::Material& material) {
		MaterialFactory::Constants constants;
		constants.baseColorFactor = toVec4(material.pbrData.baseColorFactor);
		constants.metallicRoughnessFactors = glm::vec4(
			material.pbrData.metallicFactor,
			material.pbrData.roughnessFactor,
			0.0f,
			0.0f
		);
		constants.emissiveFactor = glm::vec4(
			material.emissiveFactor[0],
			material.emissiveFactor[1],
			material.emissiveFactor[2],
			1.0f
		);
		if (material.alphaMode == fastgltf::AlphaMode::Mask) {
			constants.alphaCutoff = material.alphaCutoff;
		}
		return constants;
	}

	MaterialFactory::Resources GLTFMaterialLoader::createResources(const LoadContext& ctx) {
		// Start with default resources
		MaterialFactory::Resources resources = ctx.defaults;
		resources.uniformBuffer = ctx.constantBuffer.buffer;
		resources.uniformBufferOffset = ctx.materialIndex * sizeof(MaterialFactory::Constants);

		// Load albedo texture
		if (ctx.material->pbrData.baseColorTexture) {
			if (auto albedo = tryGetTexture(&ctx.material->pbrData.baseColorTexture.value(), ctx)) {
				resources.albedoTexture = albedo->first;
				resources.sampler = albedo->second;
			}
		}

		// Load metallic-roughness texture
		if (ctx.material->pbrData.metallicRoughnessTexture) {
			if (auto metallicRoughness = tryGetTexture(&ctx.material->pbrData.metallicRoughnessTexture.value(), ctx)) {
				resources.metallicRoughnessTexture = metallicRoughness->first;
				resources.sampler = metallicRoughness->second;
			}
		}

		// TODO: Load normal and emissive textures if needed

		return resources;
	}

	MaterialPass GLTFMaterialLoader::determinePassType(const fastgltf::Material& material) {
		return (material.alphaMode == fastgltf::AlphaMode::Blend)
			? MaterialPass::Transparent
			: MaterialPass::MainColor;
	}

	std::optional<std::pair<AllocatedImage, VkSampler>> GLTFMaterialLoader::tryGetTexture(
		const fastgltf::TextureInfo* textureInfo, const LoadContext& ctx) {
		if (!textureInfo) {
			return std::nullopt;
		}
		const auto& texture = ctx.gltf.textures[textureInfo->textureIndex];
		if (!texture.imageIndex || !texture.samplerIndex) {
			return std::nullopt;
		}
		return std::make_pair(
			ctx.images[texture.imageIndex.value()],
			ctx.samplers[texture.samplerIndex.value()]
		);
	}

	glm::vec4 GLTFMaterialLoader::toVec4(const std::array<float, 4>& arr) {
		return glm::vec4(arr[0], arr[1], arr[2], arr[3]);
	}

	// LoadedGLTF implementation
	LoadedGLTF::LoadedGLTF(Engine* engine) : creator(engine) {}

	LoadedGLTF::~LoadedGLTF() {
		clearAll();
	}

	// Draws the GLTF scene
	void LoadedGLTF::draw(const glm::mat4& topMatrix, RenderQueue& renderQueue) {
		for (const auto& node : topNodes) {
			node->draw(topMatrix, renderQueue);
		}
	}

	// Clears all resources associated with the GLTF scene
	void LoadedGLTF::clearAll() {
		VkDevice device = creator->getDevice();

		// First clear scene hierarchy to release refs
		for (auto& node : topNodes) {
			node.reset();
		}
		topNodes.clear();
		nodes.clear();

		// Now safe to destroy descriptor pool
		//descriptorPool.destroyPools(device);

		// Destroy material buffer
		creator->destroyBuffer(materialDataBuffer);

		// Cleanup meshes
		for (const auto& [name, mesh] : meshes) {
			mesh->cleanup(creator);
		}
		meshes.clear();

		// Cleanup images
		for (const auto& [name, image] : images) {
			if (image.image != creator->getDefaultEngineData().errorCheckerboardImage.image) {
				creator->destroyImage(image);
			}
		}
		images.clear();

		// Cleanup samplers last since they might be referenced by material descriptors
		for (auto sampler : samplers) {
			vkDestroySampler(device, sampler, nullptr);
		}
		samplers.clear();
	}

	// GLTFLoader implementation
	std::optional<Shared<LoadedGLTF>> GLTFLoader::loadGLTF(Engine* engine, const std::filesystem::path& filePath) {
		Shared<LoadedGLTF> scene = CreateShared<LoadedGLTF>(engine);
		LoadedGLTF& file = *scene.get();
		VkDevice device = engine->getDevice();
		auto defaultEngineData = engine->getDefaultEngineData();

		fastgltf::Parser parser{};
		constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember |
			fastgltf::Options::AllowDouble |
			fastgltf::Options::LoadGLBBuffers |
			fastgltf::Options::LoadExternalBuffers;

		fastgltf::GltfDataBuffer data;
		data.loadFromFile(filePath);

		fastgltf::Asset gltf;
		auto type = fastgltf::determineGltfFileType(&data);
		std::filesystem::path path = filePath;

		// Load GLTF or GLB file
		if (type == fastgltf::GltfType::glTF) {
			auto loadResult = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
			if (loadResult) {
				gltf = std::move(loadResult.get());
			}
			else {
				std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(loadResult.error()) << std::endl;
				return std::nullopt;
			}
		}
		else if (type == fastgltf::GltfType::GLB) {
			auto loadResult = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
			if (loadResult) {
				gltf = std::move(loadResult.get());
			}
			else {
				std::cerr << "Failed to load GLB: " << fastgltf::to_underlying(loadResult.error()) << std::endl;
				return std::nullopt;
			}
		}
		else {
			std::cerr << "Unsupported GLTF container type" << std::endl;
			return std::nullopt;
		}

		// Initialize descriptor pool for materials
		std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		};
		//file.descriptorPool.init(device, gltf.materials.size(), sizes);

		// Create samplers from GLTF samplers
		for (const auto& sampler : gltf.samplers) {
			VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
			samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
			samplerInfo.minLod = 0;
			samplerInfo.magFilter = extractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
			samplerInfo.minFilter = extractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
			samplerInfo.mipmapMode = extractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

			VkSampler vkSampler;
			vkCreateSampler(device, &samplerInfo, nullptr, &vkSampler);
			file.samplers.push_back(vkSampler);
		}

		// Load images
		std::vector<AllocatedImage> images;
		for (auto& image : gltf.images) {
			auto loadedImage = loadImage(engine, gltf, image);
			if (loadedImage) {
				images.push_back(*loadedImage);
				file.images[image.name.c_str()] = *loadedImage;
			}
			else {
				images.push_back(defaultEngineData.errorCheckerboardImage);
				std::cerr << "Failed to load texture: " << image.name << std::endl;
			}
		}

		// Setup default material resources
		const MaterialFactory::Resources defaultResources{
			.albedoTexture = defaultEngineData.whiteImage,
			.metallicRoughnessTexture = defaultEngineData.whiteImage,
			.sampler = defaultEngineData.samplerLinear,
			// TODO: Add other default resources if necessary
		};

		// Create material data buffer

		std::vector<Shared<MaterialInstance>> materials;
		if (gltf.materials.size() > 0)
		{
			file.materialDataBuffer = engine->createBuffer(
				sizeof(MaterialFactory::Constants) * gltf.materials.size(),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU
			);
			// Load materials
			GLTFMaterialLoader::LoadContext ctx{
				.engine = engine,
				.gltf = gltf,
				.constantBuffer = file.materialDataBuffer,
				.defaults = defaultResources,
				.images = images,
				.samplers = file.samplers
			};

			for (size_t i = 0; i < gltf.materials.size(); ++i) {
				ctx.material = &gltf.materials[i];
				ctx.materialIndex = i;
				auto materialInstance = CreateShared<MaterialInstance>(GLTFMaterialLoader::loadMaterial(ctx));
				materials.push_back(materialInstance);
				file.materials[gltf.materials[i].name.c_str()] = materialInstance;
			}
		}

		// Load meshes

		for (auto& mesh : gltf.meshes) {
			Shared<MeshAsset> newMesh = CreateShared<MeshAsset>();
			newMesh->setName(mesh.name.c_str());

			file.meshes[mesh.name.c_str()] = newMesh;
			std::vector<uint32_t> indices;
			std::vector<Vertex> vertices;

			for (const auto& primitive : mesh.primitives) {
				Surface newSurface;
				newSurface.startIndex = static_cast<uint32_t>(indices.size());
				newSurface.indexCount = static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);
				size_t initialVertexIndex = vertices.size();

				// Load indices
				{
					auto& indexAccessor = gltf.accessors[primitive.indicesAccessor.value()];
					indices.reserve(indices.size() + indexAccessor.count);
					fastgltf::iterateAccessor<uint32_t>(gltf, indexAccessor, [&](uint32_t idx) {
						indices.push_back(idx + static_cast<uint32_t>(initialVertexIndex));
						});
				}

				// Load positions
				{
					auto positionIt = primitive.findAttribute("POSITION");
					if (positionIt != primitive.attributes.end()) {
						auto& positionAccessor = gltf.accessors[positionIt->second];
						vertices.resize(vertices.size() + positionAccessor.count);
						fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, positionAccessor, [&](glm::vec3 v, size_t index) {
							Vertex vertex{};
							vertex.position = v;
							vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);  // Default normal
							vertex.color = glm::vec4(1.0f);
							vertex.uvX = 0.0f;
							vertex.uvY = 0.0f;
							vertices[initialVertexIndex + index] = vertex;
							});
					}
				}

				// Load normals
				auto normalIt = primitive.findAttribute("NORMAL");
				if (normalIt != primitive.attributes.end()) {
					auto& normalAccessor = gltf.accessors[normalIt->second];
					fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, normalAccessor, [&](glm::vec3 n, size_t index) {
						vertices[initialVertexIndex + index].normal = n;
						});
				}

				// Load UVs
				auto uvIt = primitive.findAttribute("TEXCOORD_0");
				if (uvIt != primitive.attributes.end()) {
					auto& uvAccessor = gltf.accessors[uvIt->second];
					fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, uvAccessor, [&](glm::vec2 uv, size_t index) {
						vertices[initialVertexIndex + index].uvX = uv.x;
						vertices[initialVertexIndex + index].uvY = uv.y;
						});
				}

				// Load vertex colors
				auto colorIt = primitive.findAttribute("COLOR_0");
				if (colorIt != primitive.attributes.end()) {
					auto& colorAccessor = gltf.accessors[colorIt->second];
					fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, colorAccessor, [&](glm::vec4 c, size_t index) {
						vertices[initialVertexIndex + index].color = c;
						});
				}

				// Assign material
				if (primitive.materialIndex) {
					newSurface.material = materials[primitive.materialIndex.value()];
				}
				else {
					newSurface.material = defaultEngineData.defaultMaterial;
				}

				newMesh->addSurface(newSurface);
			}

			// Upload mesh data to GPU
			newMesh->uploadToGPU(engine, indices, vertices);
		}

		std::vector<Shared<SceneNode>> nodes;
		nodes.reserve(gltf.nodes.size());

		for (size_t i = 0; i < gltf.nodes.size(); ++i) {
			auto& node = gltf.nodes[i];

			Shared<SceneNode> sceneNode;

			if (node.meshIndex.has_value()) {
				auto meshNode = CreateShared<MeshNode>();
				meshNode->setMesh(file.meshes[gltf.meshes[node.meshIndex.value()].name.c_str()]);
				sceneNode = meshNode;
			}
			else {
				sceneNode = CreateShared<SceneNode>();
			}

			nodes.push_back(sceneNode);
			file.nodes[node.name.c_str()] = sceneNode;

			// Set node transform
			std::visit(fastgltf::visitor{
				[&](fastgltf::Node::TransformMatrix matrix) {
					glm::mat4 localTransform;
					std::memcpy(&localTransform, matrix.data(), sizeof(matrix));
					sceneNode->setLocalTransform(localTransform);
				},
				[&](fastgltf::Node::TRS transform) {
					glm::vec3 translation(transform.translation[0], transform.translation[1], transform.translation[2]);
					glm::quat rotation(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
					glm::vec3 scale(transform.scale[0], transform.scale[1], transform.scale[2]);
					glm::mat4 localTransform = glm::translate(glm::mat4(1.0f), translation) *
											   glm::toMat4(rotation) *
											   glm::scale(glm::mat4(1.0f), scale);
					sceneNode->setLocalTransform(localTransform);
				} },
				node.transform);
		}

		for (size_t i = 0; i < gltf.nodes.size(); ++i)
		{
			auto& node = gltf.nodes[i];
			auto& sceneNode = nodes[i];
			for (auto childIndex : node.children) {
				sceneNode->addChild(nodes[childIndex]);
			}
		}

		// find the top nodes, with no parents
		for (auto& node : nodes) {
			if (node->getParent().lock() == nullptr) {
				file.topNodes.push_back(node);
				node->refreshTransform(glm::mat4{ 1.0f });
			}
		}

		return scene;
	}

	std::optional<AllocatedImage> GLTFLoader::loadImage(Engine* engine, fastgltf::Asset& asset, fastgltf::Image& image) {
		AllocatedImage newImage{};
		int width, height, nrChannels;

		std::visit(fastgltf::visitor{
			[](auto&) {},  // Ignore unhandled types
			[&](fastgltf::sources::URI& uri) {
				if (uri.uri.isLocalPath()) {
					std::string path(uri.uri.path().begin(), uri.uri.path().end());
					unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
					if (data) {
						VkExtent3D imageSize{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
						newImage = engine->createImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
						stbi_image_free(data);
					}
				}
			},
			[&](fastgltf::sources::Vector& vector) {
				unsigned char* data = stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
				if (data) {
					VkExtent3D imageSize{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
					newImage = engine->createImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
					stbi_image_free(data);
				}
			},
			[&](fastgltf::sources::BufferView& bufferViewSource) {
				auto& bufferView = asset.bufferViews[bufferViewSource.bufferViewIndex];
				auto& buffer = asset.buffers[bufferView.bufferIndex];
				std::visit(fastgltf::visitor{
					[](auto&) {},
					[&](fastgltf::sources::Vector& vector) {
						unsigned char* data = stbi_load_from_memory(vector.bytes.data() + bufferView.byteOffset, static_cast<int>(bufferView.byteLength), &width, &height, &nrChannels, 4);
						if (data) {
							VkExtent3D imageSize{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
							newImage = engine->createImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
							stbi_image_free(data);
						}
					}
				}, buffer.data);
			}
			}, image.data);

		if (newImage.image == VK_NULL_HANDLE) {
			return std::nullopt;
		}
		else {
			return newImage;
		}
	}
}  // namespace SE