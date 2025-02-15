/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include <vulkan/vulkan.h>

#pragma warning( push )
#pragma warning( disable: 4365 )
#pragma warning( disable: 5262 )
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>
#pragma warning( pop )

#if !defined( _WIN32 ) || defined( CRG_BUILD_STATIC )
#	define CRG_API
#else
#	if defined( RenderGraph_EXPORTS )
#		define CRG_API __declspec( dllexport )
#	else
#		define CRG_API __declspec( dllimport )
#	endif
#endif

namespace crg
{
	template< typename DataT >
	struct Id;

	struct Attachment;
	struct AttachmentTransitions;
	struct Buffer;
	struct FramePassTransitions;
	struct GraphContext;
	struct ImageData;
	struct ImageViewData;
	struct GraphNode;
	struct FramePass;
	struct FramePassGroup;
	struct RootNode;
	struct SamplerDesc;
	struct Texcoord;
	struct WriteDescriptorSet;

	class ContextResourcesCache;
	class Exception;
	class Fence;
	class FrameGraph;
	class FramePassTimer;
	class GraphVisitor;
	class RecordContext;
	class ResourceHandler;
	class ResourcesCache;
	class RunnableGraph;
	class RunnablePass;

	class ImageCopy;
	class PipelinePass;
	class RenderPass;
	class RenderQuad;

	using ImageId = Id< ImageData >;
	using ImageViewId = Id< ImageViewData >;
	using DependencyCache = std::unordered_map< size_t, bool >;
	using PassDependencyCache = std::unordered_map< FramePass const *, DependencyCache >;

	using FramePassPtr = std::unique_ptr< FramePass >;
	using FramePassGroupPtr = std::unique_ptr< FramePassGroup >;
	using GraphNodePtr = std::unique_ptr< GraphNode >;
	using RunnableGraphPtr = std::unique_ptr< RunnableGraph >;
	using RunnablePassPtr = std::unique_ptr< RunnablePass >;
	using GraphAdjacentNode = GraphNode *;
	using ConstGraphAdjacentNode = GraphNode const *;

	template< typename DataT >
	struct DataTransitionT;
	template< typename DataT >
	using DataTransitionArrayT = std::vector< DataTransitionT< DataT > >;
	/**
	*\brief
	*	The transition between two states of an image view.
	*/
	using ViewTransition = DataTransitionT< ImageViewId >;
	using ViewTransitionArray = DataTransitionArrayT< ImageViewId >;
	/**
	*\brief
	*	The transition between two states of a storage buffer.
	*/
	using BufferTransition = DataTransitionT< Buffer >;
	using BufferTransitionArray = DataTransitionArrayT< Buffer >;

	using AttachmentArray = std::vector< Attachment >;
	using FramePassPtrArray = std::vector< FramePassPtr >;
	using FramePassGroupPtrArray = std::vector< FramePassGroupPtr >;
	using FrameGraphArray = std::vector< FrameGraph const * >;
	using FramePassArray = std::vector< FramePass const * >;
	using GraphAdjacentNodeArray = std::vector< GraphAdjacentNode >;
	using ConstGraphAdjacentNodeArray = std::vector< ConstGraphAdjacentNode >;
	using GraphNodePtrArray = std::vector< GraphNodePtr >;
	using FramePassDependencies = std::vector< FramePassTransitions >;
	using WriteDescriptorSetArray = std::vector< WriteDescriptorSet >;
	using AttachmentsNodeMap = std::map< ConstGraphAdjacentNode, AttachmentTransitions >;
	using ImageMemoryMap = std::map< ImageId, std::pair< VkImage, VkDeviceMemory > >;
	using ImageViewMap = std::map< ImageViewId, VkImageView >;
	using ImageIdArray = std::vector< ImageId >;
	using ImageViewIdArray = std::vector< ImageViewId >;

	template< typename DataT >
	using IdAliasMap = std::map< Id< DataT >, Id< DataT > >;
	using ImageIdAliasMap = IdAliasMap< ImageData >;
	using ImageViewIdAliasMap = IdAliasMap< ImageViewData >;

	template< typename DataT >
	using IdDataOwnerCont = std::map< Id< DataT >, std::unique_ptr< DataT > >;
	using ImageIdDataOwnerCont = IdDataOwnerCont< ImageData >;
	using ImageViewIdDataOwnerCont = IdDataOwnerCont< ImageViewData >;

	using VkAttachmentDescriptionArray = std::vector< VkAttachmentDescription >;
	using VkAttachmentReferenceArray = std::vector< VkAttachmentReference >;
	using VkBufferArray = std::vector< VkBuffer >;
	using VkBufferViewArray = std::vector< VkBufferView >;
	using VkDescriptorBufferInfoArray = std::vector< VkDescriptorBufferInfo >;
	using VkDescriptorImageInfoArray = std::vector< VkDescriptorImageInfo >;
	using VkDescriptorSetLayoutBindingArray = std::vector< VkDescriptorSetLayoutBinding >;
	using VkDescriptorPoolSizeArray = std::vector< VkDescriptorPoolSize >;
	using VkImageViewArray = std::vector< VkImageView >;
	using VkPipelineColorBlendAttachmentStateArray = std::vector< VkPipelineColorBlendAttachmentState >;
	using VkPipelineShaderStageCreateInfoArray = std::vector< VkPipelineShaderStageCreateInfo >;
	using VkPushConstantRangeArray = std::vector< VkPushConstantRange >;
	using VkScissorArray = std::vector< VkRect2D >;
	using VkSubpassDependencyArray = std::vector< VkSubpassDependency >;
	using VkVertexInputAttributeDescriptionArray = std::vector< VkVertexInputAttributeDescription >;
	using VkVertexInputBindingDescriptionArray = std::vector< VkVertexInputBindingDescription >;
	using VkViewportArray = std::vector< VkViewport >;
	using VkWriteDescriptorSetArray = std::vector< VkWriteDescriptorSet >;

	struct PipelineState
	{
		VkAccessFlags access;
		VkPipelineStageFlags pipelineStage;
	};

	struct LayoutState
	{
		VkImageLayout layout;
		PipelineState state;
	};

	using AccessState = PipelineState;

	using MipLayoutStates = std::map< uint32_t, LayoutState >;
	using LayerLayoutStates = std::map< uint32_t, MipLayoutStates >;
	using LayoutStateMap = std::unordered_map< uint32_t, LayerLayoutStates >;
	using LayerLayoutStatesMap = std::map< uint32_t, LayerLayoutStates >;
	using AccessStateMap = std::unordered_map< VkBuffer, AccessState >;
	using ViewsLayout = LayoutStateMap;
	using BuffersLayout = AccessStateMap;
	using ViewsLayoutPtr = std::unique_ptr< ViewsLayout >;
	using BuffersLayoutPtr = std::unique_ptr< BuffersLayout >;
	using ViewsLayouts = std::vector< ViewsLayoutPtr >;
	using BuffersLayouts = std::vector< BuffersLayoutPtr >;

	using ViewLayoutIterators = std::map< uint32_t, ViewsLayouts::iterator >;
	using BufferLayoutIterators = std::map< uint32_t, BuffersLayouts::iterator >;

	template< typename VkTypeT >
	struct ContextObjectT
	{
		ContextObjectT( ContextObjectT const & rhs ) = delete;
		ContextObjectT( ContextObjectT && rhs )noexcept = delete;
		ContextObjectT & operator=( ContextObjectT const & rhs ) = delete;
		ContextObjectT & operator=( ContextObjectT && rhs )noexcept = delete;

		explicit ContextObjectT( GraphContext & ctx
			, VkTypeT obj = {}
			, void( *dtor )( GraphContext &, VkTypeT & )noexcept = nullptr )
			: context{ ctx }
			, object{ obj }
			, destroy{ dtor }
		{
		}

		~ContextObjectT()noexcept
		{
			if ( destroy && object )
			{
				destroy( context, object );
			}
		}

		GraphContext & context;
		VkTypeT object;
		void ( *destroy )( GraphContext &, VkTypeT & )noexcept;
	};

	struct Buffer
	{
		std::string name;

		Buffer( VkBufferArray pbuffers
			, std::string pname )noexcept
			: name{ std::move( pname ) }
			, m_buffers{ std::move( pbuffers ) }
		{
		}

		Buffer( VkBuffer buffer
			, std::string name )noexcept
			: Buffer{ VkBufferArray{ buffer }, std::move( name ) }
		{
		}

		VkBuffer const & buffer( uint32_t index = 0 )const noexcept
		{
			return m_buffers.size() == 1u
				? m_buffers.front()
				: m_buffers[index];
		}

		VkBuffer & buffer( uint32_t index = 0 )noexcept
		{
			return m_buffers.size() == 1u
				? m_buffers.front()
				: m_buffers[index];
		}

		size_t getCount()const noexcept
		{
			return m_buffers.size();
		}

	private:
		VkBufferArray m_buffers;

		friend CRG_API bool operator==( Buffer const & lhs, Buffer const & rhs );
	};

	CRG_API bool operator==( Buffer const & lhs, Buffer const & rhs );

	struct VertexBuffer
	{
		VertexBuffer( VertexBuffer const & rhs ) = delete;
		VertexBuffer & operator=( VertexBuffer const & rhs ) = delete;
		~VertexBuffer()noexcept = default;

		explicit VertexBuffer( Buffer pbuffer = { VkBuffer{}, std::string{} }
			, VkDeviceMemory pmemory = VkDeviceMemory{}
			, VkVertexInputAttributeDescriptionArray pvertexAttribs = {}
			, VkVertexInputBindingDescriptionArray pvertexBindings = {} )
			: buffer{ std::move( pbuffer ) }
			, memory{ pmemory }
			, vertexAttribs{ std::move( pvertexAttribs ) }
			, vertexBindings{ std::move( pvertexBindings ) }
		{
			inputState.vertexAttributeDescriptionCount = uint32_t( vertexAttribs.size() );
			inputState.pVertexAttributeDescriptions = vertexAttribs.data();
			inputState.vertexBindingDescriptionCount = uint32_t( vertexBindings.size() );
			inputState.pVertexBindingDescriptions = vertexBindings.data();
		}

		VertexBuffer( VertexBuffer && rhs )noexcept
			: buffer{ rhs.buffer }
			, memory{ rhs.memory }
			, vertexAttribs{ std::move( rhs.vertexAttribs ) }
			, vertexBindings{ std::move( rhs.vertexBindings ) }
			, inputState{ std::move( rhs.inputState ) }
		{
			inputState.vertexAttributeDescriptionCount = uint32_t( vertexAttribs.size() );
			inputState.pVertexAttributeDescriptions = vertexAttribs.data();
			inputState.vertexBindingDescriptionCount = uint32_t( vertexBindings.size() );
			inputState.pVertexBindingDescriptions = vertexBindings.data();
		}

		VertexBuffer & operator=( VertexBuffer && rhs )noexcept
		{
			buffer = rhs.buffer;
			memory = rhs.memory;
			vertexAttribs = std::move( rhs.vertexAttribs );
			vertexBindings = std::move( rhs.vertexBindings );
			inputState = std::move( rhs.inputState );

			inputState.vertexAttributeDescriptionCount = uint32_t( vertexAttribs.size() );
			inputState.pVertexAttributeDescriptions = vertexAttribs.data();
			inputState.vertexBindingDescriptionCount = uint32_t( vertexBindings.size() );
			inputState.pVertexBindingDescriptions = vertexBindings.data();

			return *this;
		}

		Buffer buffer{ VkBuffer{}, std::string{} };
		VkDeviceMemory memory{};
		VkVertexInputAttributeDescriptionArray vertexAttribs{};
		VkVertexInputBindingDescriptionArray vertexBindings{};
		VkPipelineVertexInputStateCreateInfo inputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, {}, {}, {}, {}, {} };
	};

	using VertexBufferPtr = std::unique_ptr< VertexBuffer >;

	struct IndexBuffer
	{
		explicit IndexBuffer( Buffer pbuffer = { VkBuffer{}, std::string{} }
			, VkDeviceMemory pmemory = VkDeviceMemory{} )
			: buffer{ std::move( pbuffer ) }
			, memory{ pmemory }
		{
		}

		Buffer buffer;
		VkDeviceMemory memory;
	};

	struct IndirectBuffer
	{
		explicit IndirectBuffer( Buffer pbuffer
			, uint32_t pstride
			, VkDeviceSize poffset = {} )
			: buffer{ std::move( pbuffer ) }
			, offset{ poffset }
			, stride{ pstride }
		{
		}

		Buffer buffer;
		VkDeviceSize offset;
		uint32_t stride;
	};

	static constexpr VkPipelineColorBlendAttachmentState DefaultBlendState{ VK_FALSE
		, VK_BLEND_FACTOR_ONE
		, VK_BLEND_FACTOR_ZERO
		, VK_BLEND_OP_ADD
		, VK_BLEND_FACTOR_ONE
		, VK_BLEND_FACTOR_ZERO
		, VK_BLEND_OP_ADD
		, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };

	template< typename TypeT >
	struct DefaultValueGetterT;

	template< typename TypeT >
	static inline const TypeT defaultV = DefaultValueGetterT< TypeT >::get();

	template< typename TypeT >
	static inline TypeT getDefaultV()
	{
		return DefaultValueGetterT< TypeT >::get();
	}

	template<>
	struct DefaultValueGetterT< VkPipelineVertexInputStateCreateInfo >
	{
		static VkPipelineVertexInputStateCreateInfo get()
		{
			VkPipelineVertexInputStateCreateInfo const result{ []()
				{
					return VkPipelineVertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
						, nullptr
						, {}
						, {}
						, {}
						, {}
						, {} };
				}() };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< IndirectBuffer >
	{
		static IndirectBuffer get()
		{
			IndirectBuffer const result{ Buffer{ VkBuffer{}, std::string{} }, 0u };
			return result;
		}
	};

	template< typename TypeT >
	struct RawTyperT
	{
		using Type = TypeT;
	};

	template< typename TypeT >
	using RawTypeT = typename RawTyperT< TypeT >::Type;

	struct SemaphoreWait
	{
		VkSemaphore semaphore;
		VkPipelineStageFlags dstStageMask;
	};

	using SemaphoreWaitArray = std::vector< SemaphoreWait >;

	CRG_API VkExtent3D getExtent( ImageId const & image );
	CRG_API VkExtent3D getExtent( ImageViewId const & image );
	CRG_API VkExtent3D getMipExtent( ImageViewId const & image );
	CRG_API VkFormat getFormat( ImageId const & image );
	CRG_API VkFormat getFormat( ImageViewId const & image );
	CRG_API VkImageType getImageType( ImageId const & image );
	CRG_API VkImageType getImageType( ImageViewId const & image );
	CRG_API uint32_t getMipLevels( ImageId const & image );
	CRG_API uint32_t getMipLevels( ImageViewId const & image );
	CRG_API uint32_t getArrayLayers( ImageId const & image );
	CRG_API uint32_t getArrayLayers( ImageViewId const & image );
	CRG_API VkAccessFlags getAccessMask( VkImageLayout layout )noexcept;
	CRG_API VkPipelineStageFlags getStageMask( VkImageLayout layout )noexcept;
	CRG_API PipelineState getPipelineState( VkPipelineStageFlags flags )noexcept;
	CRG_API LayoutState makeLayoutState( VkImageLayout layout );
	CRG_API VkImageAspectFlags getAspectMask( VkFormat format )noexcept;
	CRG_API LayoutState const & addSubresourceRangeLayout( LayerLayoutStates & ranges
		, VkImageSubresourceRange const & range
		, LayoutState const & newLayout );
	CRG_API LayoutState getSubresourceRangeLayout( LayerLayoutStates const & ranges
		, VkImageSubresourceRange const & range );
	CRG_API VkImageSubresourceRange getVirtualRange( ImageId const & image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & range )noexcept;
	CRG_API bool match( ImageViewData const & lhs, ImageViewData const & rhs )noexcept;
	CRG_API bool isDepthFormat( VkFormat fmt )noexcept;
	CRG_API bool isStencilFormat( VkFormat fmt )noexcept;
	CRG_API bool isColourFormat( VkFormat fmt )noexcept;
	CRG_API bool isDepthStencilFormat( VkFormat fmt )noexcept;
	CRG_API ImageViewId const & resolveView( ImageViewId const & view
		, uint32_t passIndex );

	CRG_API void convert( SemaphoreWaitArray const & toWait
		, std::vector< VkSemaphore > & semaphores
		, std::vector< VkPipelineStageFlags > & dstStageMasks );
	CRG_API VkQueryPool createQueryPool( GraphContext & context
		, std::string const & name
		, uint32_t passesCount );
}
