/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/GraphNode.hpp"

#include "RenderGraph/GraphVisitor.hpp"
#include "RenderGraph/FramePass.hpp"

namespace crg
{
	//*********************************************************************************************

	GraphNode::GraphNode( Kind kind
		, std::string name )
		: kind{ kind }
		, name{ std::move( name ) }
		, next{}
	{
	}

	void GraphNode::addAttaches( GraphAdjacentNode prev
		, AttachmentTransitionArray inAttaches )
	{
		bool dirty = false;
		auto * mine = &this->inputAttaches[prev];

		for ( auto & attach : inAttaches )
		{
			auto it = std::find( mine->begin()
				, mine->end()
				, attach );

			if ( it == mine->end() )
			{
				mine->push_back( std::move( attach ) );
				dirty = true;
			}
		}

		if ( dirty )
		{
			*mine = mergeIdenticalTransitions( std::move( *mine ) );
		}
	}

	void GraphNode::attachNode( GraphAdjacentNode next
		, AttachmentTransitionArray inputAttaches )
	{
		auto it = std::find( this->next.begin()
			, this->next.end()
			, next );

		if ( it == this->next.end() )
		{
			this->next.push_back( next );
		}

		next->addAttaches( this
			, std::move( inputAttaches ) );
	}

	GraphAdjacentNode GraphNode::findInNext( FramePass const & pass )const
	{
		auto it = std::find_if( next.begin()
			, next.end()
			, [&pass]( GraphAdjacentNode lookup )
			{
				return getFramePass( *lookup ) == &pass;
			} );
		return ( next.end() != it )
			? *it
			: nullptr;
	}

	AttachmentTransitionArray const & GraphNode::getInputAttaches( ConstGraphAdjacentNode const pred )const
	{
		auto it = inputAttaches.find( pred );
		assert( it != inputAttaches.end() );
		return it->second;
	}

	//*********************************************************************************************

	FramePassNode::FramePassNode( FramePass const & pass )
		: GraphNode{ MyKind, pass.name }
		, pass{ &pass }
	{
	}

	void FramePassNode::accept( GraphVisitor * vis )const
	{
		vis->visitFramePassNode( this );
	}

	//*********************************************************************************************

	RootNode::RootNode( std::string name )
		: GraphNode{ MyKind, std::move( name ) }
	{
	}

	void RootNode::accept( GraphVisitor * vis )const
	{
		vis->visitRootNode( this );
	}

	//*********************************************************************************************

	FramePass const * getFramePass( GraphNode const & node )
	{
		if ( !isFramePassNode( node ) )
		{
			return nullptr;
		}

		return &nodeCast< FramePassNode >( node ).getFramePass();
	}

	//*********************************************************************************************
}
