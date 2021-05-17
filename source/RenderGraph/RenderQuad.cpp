/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RenderQuad.hpp"

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	namespace
	{
		VkAttachmentReference addAttach( Attachment const & attach
			, VkAttachmentDescriptionArray & attaches )
		{
			VkImageLayout attachLayout;

			if ( attach.hasFlag( Attachment::Flag::Depth ) )
			{
				if ( attach.isDepthOutput() && attach.isStencilOutput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else if ( attach.isDepthOutput() && attach.isStencilInput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
				}
				else if ( attach.isDepthInput() && attach.isStencilOutput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else if ( attach.isDepthInput() && attach.isStencilInput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
				else if ( attach.isDepthOutput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				}
				else if ( attach.isStencilOutput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else if ( attach.isDepthInput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
				}
				else if ( attach.isStencilInput() )
				{
					attachLayout = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
				}
			}
			else
			{
				attachLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}

			VkAttachmentReference result{ uint32_t( attaches.size() )
				, attachLayout };
			attaches.push_back( { 0u
				, attach.viewData.format
				, attach.viewData.image.data->samples
				, attach.loadOp
				, attach.storeOp
				, attach.stencilLoadOp
				, attach.stencilStoreOp
				, attach.initialLayout
				, attach.finalLayout } );
			return result;
		}
	}

	RenderQuad::RenderQuad( RenderPass const & pass
		, GraphContext const & context
		, RunnableGraph const & graph
		, rq::Config config )
		: RunnablePass{ pass
			, context
			, graph
			, std::move( config.baseConfig )
			, VK_PIPELINE_BIND_POINT_GRAPHICS }
		, m_config{ std::move( config.texcoordConfig ? *config.texcoordConfig : defaultV< rq::Texcoord > )
			, std::move( config.renderSize ? *config.renderSize : defaultV< VkExtent2D > )
			, std::move( config.renderPosition ? *config.renderPosition : defaultV< VkOffset2D > ) }
		, m_useTexCoord{ config.texcoordConfig }
	{
	}

	RenderQuad::~RenderQuad()
	{
		if ( m_vertexMemory )
		{
			crgUnregisterObject( m_context, m_vertexMemory );
			m_context.vkFreeMemory( m_context.device
				, m_vertexMemory
				, m_context.allocator );
		}

		if ( m_vertexBuffer )
		{
			crgUnregisterObject( m_context, m_vertexBuffer );
			m_context.vkDestroyBuffer( m_context.device
				, m_vertexBuffer
				, m_context.allocator );
		}

		if ( m_frameBuffer )
		{
			crgUnregisterObject( m_context, m_frameBuffer );
			m_context.vkDestroyFramebuffer( m_context.device
				, m_frameBuffer
				, m_context.allocator );
		}

		if ( m_renderPass )
		{
			crgUnregisterObject( m_context, m_renderPass );
			m_context.vkDestroyRenderPass( m_context.device
				, m_renderPass
				, m_context.allocator );
		}
	}

	void RenderQuad::recordInto( VkCommandBuffer commandBuffer )const
	{
		static VkDeviceSize const offsets = 0u;
		RunnablePass::recordInto( commandBuffer );
		m_context.vkCmdBindVertexBuffers( commandBuffer, 0u, 1u, &m_vertexBuffer, &offsets );
		m_context.vkCmdDraw( commandBuffer, 4u, 1u, 0u, 0u );
	}

	void RenderQuad::doInitialise()
	{
		doCreateVertexBuffer();
		doCreateVertexMemory();
		doCreateRenderPass();
		doCreatePipeline();
		doCreateFramebuffer();
	}

	void RenderQuad::doCreateVertexBuffer()
	{
		VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
			, nullptr
			, 0u
			, 4u * sizeof( Quad::Vertex )
			, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
			, VK_SHARING_MODE_EXCLUSIVE
			, 0u
			, nullptr };
		auto res = m_context.vkCreateBuffer( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_vertexBuffer );
		checkVkResult( res, "Buffer creation" );
		crgRegisterObject( m_context, m_pass.name, m_vertexBuffer );
	}

	void RenderQuad::doCreateVertexMemory()
	{
		VkMemoryRequirements requirements{};
		m_context.vkGetBufferMemoryRequirements( m_context.device
			, m_vertexBuffer
			, &requirements );
		uint32_t deduced = m_context.deduceMemoryType( requirements.memoryTypeBits
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
		VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
			, nullptr
			, requirements.size
			, deduced };
		auto res = m_context.vkAllocateMemory( m_context.device
			, &allocateInfo
			, m_context.allocator
			, &m_vertexMemory );
		checkVkResult( res, "Buffer memory allocation" );
		crgRegisterObject( m_context, m_pass.name, m_vertexMemory );

		res = m_context.vkBindBufferMemory( m_context.device
			, m_vertexBuffer
			, m_vertexMemory
			, 0u );
		checkVkResult( res, "Buffer memory binding" );

		Quad::Vertex * buffer{};
		res = m_context.vkMapMemory( m_context.device
			, m_vertexMemory
			, 0u
			, VK_WHOLE_SIZE
			, 0u
			, reinterpret_cast< void ** >( &buffer ) );
		checkVkResult( res, "Buffer memory mapping" );

		if ( buffer )
		{
			std::array< Quad::Vertex, 4u > vertexData{ Quad::Vertex{ { -1.0, -1.0 }
					, ( m_useTexCoord
						? Quad::Data{ ( m_config.texcoordConfig.invertU ? 1.0f : 0.0f ), ( m_config.texcoordConfig.invertV ? 1.0f : 0.0f ) }
						: Quad::Data{ 0.0f, 0.0f } ) }
				, Quad::Vertex{ { -1.0, +1.0 }
					, ( m_useTexCoord
						? Quad::Data{ ( m_config.texcoordConfig.invertU ? 1.0f : 0.0f ), ( m_config.texcoordConfig.invertV ? 0.0f : 1.0f ) }
						: Quad::Data{ 0.0f, 0.0f } ) }
				, Quad::Vertex{ { +1.0f, -1.0f }
					, ( m_useTexCoord
						? Quad::Data{ ( m_config.texcoordConfig.invertU ? 0.0f : 1.0f ), ( m_config.texcoordConfig.invertV ? 1.0f : 0.0f ) }
						: Quad::Data{ 0.0f, 0.0f } ) }
				, Quad::Vertex{ { +1.0f, +1.0f }
					, ( m_useTexCoord
						? Quad::Data{ ( m_config.texcoordConfig.invertU ? 0.0f : 1.0f ), ( m_config.texcoordConfig.invertV ? 0.0f : 1.0f ) }
						: Quad::Data{ 0.0f, 0.0f } ) } };
			std::copy( vertexData.begin(), vertexData.end(), buffer );

			VkMappedMemoryRange memoryRange{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE
				, 0u
				, m_vertexMemory
				, 0u
				, VK_WHOLE_SIZE };
			m_context.vkFlushMappedMemoryRanges( m_context.device, 1u, &memoryRange );
			m_context.vkUnmapMemory( m_context.device, m_vertexMemory );
		}
	}

	void RenderQuad::doCreateRenderPass()
	{
		VkAttachmentDescriptionArray attaches;
		VkAttachmentReferenceArray colorReferences;
		VkAttachmentReference depthReference{};

		if ( m_pass.depthStencilInOut )
		{
			depthReference = addAttach( *m_pass.depthStencilInOut, attaches );
		}

		for ( auto & attach : m_pass.colourInOuts )
		{
			colorReferences.push_back( addAttach( attach, attaches ) );
		}

		VkSubpassDescription subpassDesc{ 0u
			, m_bindingPoint
			, 0u
			, nullptr
			, uint32_t( colorReferences.size() )
			, colorReferences.data()
			, nullptr
			, depthReference.layout ? &depthReference : nullptr
			, 0u
			, nullptr };
		VkSubpassDependencyArray dependencies{
			{ VK_SUBPASS_EXTERNAL
				, 0u
				, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
				, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				, VK_ACCESS_MEMORY_READ_BIT
				, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
				, VK_DEPENDENCY_BY_REGION_BIT }
			, { 0u
				, VK_SUBPASS_EXTERNAL
				, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
				, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
				, VK_ACCESS_MEMORY_READ_BIT
				, VK_DEPENDENCY_BY_REGION_BIT } };
		VkRenderPassCreateInfo createInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( attaches.size() )
			, attaches.data()
			, 1u
			, &subpassDesc
			, uint32_t( dependencies.size() )
			, dependencies.data() };
		auto res = m_context.vkCreateRenderPass( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_renderPass );
		checkVkResult( res, "RenderPass creation" );
		crgRegisterObject( m_context, m_pass.name, m_renderPass );
	}

	void RenderQuad::doCreatePipeline()
	{
		VkVertexInputAttributeDescriptionArray vertexAttribs;
		VkVertexInputBindingDescriptionArray vertexBindings;
		VkViewportArray viewports;
		VkScissorArray scissors;
		VkPipelineColorBlendAttachmentStateArray blendAttachs;
		auto viState = doCreateVertexInputState( vertexAttribs, vertexBindings );
		auto vpState = doCreateViewportState( viewports, scissors );
		auto cbState = doCreateBlendState( blendAttachs );
		VkPipelineInputAssemblyStateCreateInfo iaState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
			, VK_FALSE };
		VkPipelineRasterizationStateCreateInfo rsState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_FALSE
			, VK_FALSE
			, VK_POLYGON_MODE_FILL
			, VK_CULL_MODE_NONE
			, VK_FRONT_FACE_COUNTER_CLOCKWISE
			, VK_FALSE
			, 0.0f
			, 0.0f
			, 0.0f
			, 0.0f };
		VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( m_baseConfig.program.size() )
			, m_baseConfig.program.data()
			, &viState
			, &iaState
			, nullptr
			, &vpState
			, &rsState
			, nullptr
			, &m_baseConfig.dsState
			, &cbState
			, nullptr
			, m_pipelineLayout
			, m_renderPass
			, 0u
			, VK_NULL_HANDLE
			, 0u };
		auto res = m_context.vkCreateGraphicsPipelines( m_context.device
			, m_context.cache
			, 1u
			, &createInfo
			, m_context.allocator
			, &m_pipeline );
		checkVkResult( res, "Pipeline creation" );
		crgRegisterObject( m_context, m_pass.name, m_pipeline );
	}

	VkPipelineVertexInputStateCreateInfo RenderQuad::doCreateVertexInputState( VkVertexInputAttributeDescriptionArray & vertexAttribs
		, VkVertexInputBindingDescriptionArray & vertexBindings )
	{
		vertexAttribs.push_back( { 0u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( Quad::Vertex, position ) } );

		if ( m_useTexCoord )
		{
			vertexAttribs.push_back( { 1u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( Quad::Vertex, texture ) } );
		}

		vertexBindings.push_back( { 0u, sizeof( Quad::Vertex ), VK_VERTEX_INPUT_RATE_VERTEX } );
		return { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( vertexBindings.size() )
			, vertexBindings.data()
			, uint32_t( vertexAttribs.size() )
			, vertexAttribs.data() };
	}

	VkPipelineViewportStateCreateInfo RenderQuad::doCreateViewportState( VkViewportArray & viewports
		, VkScissorArray & scissors )
	{
		viewports.push_back( { float( m_config.renderPosition.x )
			, float( m_config.renderPosition.y )
			, float( m_config.renderSize.width )
			, float( m_config.renderSize.height ) } );
		scissors.push_back( { m_config.renderPosition.x
			, m_config.renderPosition.y
			, m_config.renderSize.width
			, m_config.renderSize.height } );
		return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( viewports.size())
			, viewports.data()
			, uint32_t( scissors.size())
			, scissors.data() };
	}

	VkPipelineColorBlendStateCreateInfo RenderQuad::doCreateBlendState( VkPipelineColorBlendAttachmentStateArray & blendAttachs )
	{
		VkPipelineColorBlendAttachmentState attach{};
		attach.colorBlendOp = VK_BLEND_OP_ADD;
		attach.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		attach.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		attach.alphaBlendOp = VK_BLEND_OP_ADD;
		attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		attach.colorWriteMask = { VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };
		blendAttachs.resize( m_pass.colourInOuts.size(), attach );
		return { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_FALSE
			, VK_LOGIC_OP_COPY
			, uint32_t( blendAttachs.size() )
			, blendAttachs.data() };
	}

	void RenderQuad::doCreateFramebuffer()
	{
		VkImageViewArray attachments;
		uint32_t width{};
		uint32_t height{};
		uint32_t layers{ 1u };

		if ( m_pass.depthStencilInOut )
		{
			attachments.push_back( m_graph.getImageView( *m_pass.depthStencilInOut ) );
			width = m_pass.depthStencilInOut->viewData.image.data->extent.width;
			height = m_pass.depthStencilInOut->viewData.image.data->extent.height;
		}

		for ( auto & attach : m_pass.colourInOuts )
		{
			attachments.push_back( m_graph.getImageView( attach ) );
			width = attach.viewData.image.data->extent.width;
			height = attach.viewData.image.data->extent.height;
		}

		VkFramebufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO
			, nullptr
			, 0u
			, m_renderPass
			, uint32_t( attachments.size() )
			, attachments.data()
			, width
			, height
			, layers };
		auto res = m_context.vkCreateFramebuffer( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_frameBuffer );
		checkVkResult( res, "Framebuffer creation" );
		crgRegisterObject( m_context, m_pass.name, m_frameBuffer );
	}
}
