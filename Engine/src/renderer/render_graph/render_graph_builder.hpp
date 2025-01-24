#pragma once
#include "rhi/types.hpp"
#include"render_graph.hpp"
namespace SE
{
	enum class RGBuilderFlag
	{
		None,
		ShaderStagePS,
		ShaderStageNonPS,
	};

	class RGBuilder
	{
	public:
		RGBuilder(RenderGraph* graph, RenderGraphPassBase* pass)
		{
			m_pGraph = graph;
			m_pPass = pass;
		}

		void skipCulling() { m_pPass->markTarget(); }

		template<typename Resource>
		RGHandle create(const typename Resource::Desc& desc, const std::string& name)
		{
			return m_pGraph->create<Resource>(desc, name);
		}

		RGHandle import(rhi::ITexture* texture, rhi::ResourceAccessFlags state)
		{
			return m_pGraph->import(texture, state);
		}

		RGHandle read(const RGHandle& input, rhi::ResourceAccessFlags usage, uint32_t subresource)
		{
			SE_ASSERT(rhi::anySet(usage, (rhi::ResourceAccessFlags::MaskShaderStorage | rhi::ResourceAccessFlags::IndirectArgs | rhi::ResourceAccessFlags::TransferSrc)));

			SE_ASSERT(rhi::RHI_ALL_SUB_RESOURCE != subresource); //RG doesn't support SE_ALL_SUB_RESOURCE currently

			return m_pGraph->read(m_pPass, input, usage, subresource);
		}

		RGHandle read(const RGHandle& input, uint32_t subresource = 0, RGBuilderFlag flag = RGBuilderFlag::None)
		{
			rhi::ResourceAccessFlags state;

			switch (m_pPass->getType())
			{
			case RenderPassType::Graphics:
				if (flag == RGBuilderFlag::ShaderStagePS)
				{
					state = rhi::ResourceAccessFlags::PixelShaderRead;
				}
				else if (flag == RGBuilderFlag::ShaderStageNonPS)
				{
					state = rhi::ResourceAccessFlags::VertexShaderRead;
				}
				else
				{
					state = rhi::ResourceAccessFlags::PixelShaderRead | rhi::ResourceAccessFlags::VertexShaderRead;
				}
				break;
			case RenderPassType::Compute:
			case RenderPassType::AsyncCompute:
				state = rhi::ResourceAccessFlags::ComputeShaderRead;
				break;
			case RenderPassType::Copy:
				state = rhi::ResourceAccessFlags::TransferSrc;
				break;
			default:
				SE_ASSERT(false);
				break;
			}
			return read(input, state, subresource);
		}

		RGHandle write(const RGHandle& input, rhi::ResourceAccessFlags usage, uint32_t subresource)
		{
			SE_ASSERT(rhi::anySet(usage, (rhi::ResourceAccessFlags::MaskShaderStorage | rhi::ResourceAccessFlags::TransferDst)));

			SE_ASSERT(rhi::RHI_ALL_SUB_RESOURCE != subresource); //RG doesn't support SE_ALL_SUB_RESOURCE currently

			return m_pGraph->write(m_pPass, input, usage, subresource);
		}

		RGHandle write(const RGHandle& input, uint32_t subresource = 0, RGBuilderFlag flag = RGBuilderFlag::None)
		{
			rhi::ResourceAccessFlags state;

			switch (m_pPass->getType())
			{
			case RenderPassType::Graphics:
				if (flag == RGBuilderFlag::ShaderStagePS)
				{
					state = rhi::ResourceAccessFlags::PixelShaderStorage;
				}
				else if (flag == RGBuilderFlag::ShaderStageNonPS)
				{
					state = rhi::ResourceAccessFlags::VertexShaderStorage;
				}
				else
				{
					state = rhi::ResourceAccessFlags::PixelShaderStorage | rhi::ResourceAccessFlags::VertexShaderStorage;
				}
				break;
			case RenderPassType::Compute:
			case RenderPassType::AsyncCompute:
				state = rhi::ResourceAccessFlags::ComputeShaderStorage | rhi::ResourceAccessFlags::StorageClear;
				break;
			case RenderPassType::Copy:
				state = rhi::ResourceAccessFlags::TransferDst;
				break;
			default:
				SE_ASSERT(false);
				break;
			}
			return write(input, state, subresource);
		}

		RGHandle writeColor(uint32_t color_index, const RGHandle& input, uint32_t subresource, rhi::RenderPassLoadOp load_op, glm::vec4 clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
		{
			SE_ASSERT(m_pPass->getType() == RenderPassType::Graphics);
			return m_pGraph->writeColor(m_pPass, color_index, input, subresource, load_op, clear_color);
		}

		RGHandle writeDepth(const RGHandle& input, uint32_t subresource, rhi::RenderPassLoadOp depth_load_op, float clear_depth = 0.0f)
		{
			SE_ASSERT(m_pPass->getType() == RenderPassType::Graphics);
			return m_pGraph->writeDepth(m_pPass, input, subresource, depth_load_op, rhi::RenderPassLoadOp::DontCare, clear_depth, 0);
		}

		RGHandle writeDepth(const RGHandle& input, uint32_t subresource, rhi::RenderPassLoadOp depth_load_op, rhi::RenderPassLoadOp stencil_load_op, float clear_depth = 0.0f, uint32_t clear_stencil = 0)
		{
			SE_ASSERT(m_pPass->getType() == RenderPassType::Graphics);
			return m_pGraph->writeDepth(m_pPass, input, subresource, depth_load_op, stencil_load_op, clear_depth, clear_stencil);
		}

		RGHandle readDepth(const RGHandle& input, uint32_t subresource)
		{
			SE_ASSERT(m_pPass->getType() == RenderPassType::Graphics);
			return m_pGraph->readDepth(m_pPass, input, subresource);
		}
	private:
		RGBuilder(RGBuilder const&) = delete;
		RGBuilder& operator=(RGBuilder const&) = delete;

	private:
		RenderGraph* m_pGraph = nullptr;
		RenderGraphPassBase* m_pPass = nullptr;
	};
}