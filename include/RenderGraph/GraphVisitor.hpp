/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/GraphNode.hpp"

namespace crg
{
	class GraphVisitor
	{
	public:
		virtual ~GraphVisitor() = default;
		virtual void visitRootNode( RootNode const * node ) = 0;
		virtual void visitFramePassNode( FramePassNode const * node ) = 0;
	};
}
