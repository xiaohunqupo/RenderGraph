/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

namespace crg
{
	class ImageToBufferCopy
		: public RunnablePass
	{
	public:
		CRG_API ImageToBufferCopy( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, VkOffset3D copyOffset
			, VkExtent3D copySize
			, ru::Config ruConfig = {}
			, GetPassIndexCallback passIndex = GetPassIndexCallback( [](){ return 0u; } )
			, IsEnabledCallback isEnabled = IsEnabledCallback( [](){ return true; } ) );

	private:
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );

	private:
		VkOffset3D m_copyOffset;
		VkExtent3D m_copySize;
	};
}
