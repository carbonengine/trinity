// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepRenderObject.h"

using namespace Tr2RenderContextEnum;

namespace
{
const TriBatchType s_allTypes[] = {
	TRIBATCHTYPE_OPAQUE,
	TRIBATCHTYPE_DECAL,
	TRIBATCHTYPE_TRANSPARENT,
	TRIBATCHTYPE_ADDITIVE
};

static const Tr2EffectStateManager::RenderingMode s_renderingModes[] = {
	Tr2EffectStateManager::RM_OPAQUE,
	Tr2EffectStateManager::RM_DECAL,
	Tr2EffectStateManager::RM_ALPHA,
	Tr2EffectStateManager::RM_ALPHA_ADDITIVE
};
}

TriStepRenderObject::TriStepRenderObject( IRoot* lockobj )
{
	auto allocator = Tr2Renderer::GetPoolAllocator();
	for( unsigned i = 0; i != 4; ++i )
	{
		m_batches[s_allTypes[i]] = CCP_NEW( "TriStepRenderObject/batch" ) TriRenderBatchAccumulator<>( allocator );
		m_typeEnabled[i] = true;
	}
}

TriStepRenderObject::~TriStepRenderObject()
{
	for( auto it = m_batches.begin(); it != m_batches.end(); ++it )
	{
		CCP_DELETE( it->second );
	}
}

TriStepResult TriStepRenderObject::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( !m_renderable )
	{
		return RS_OK;
	}

	::GetBatchesFromRenderables( &m_renderable, 1, nullptr, m_batches, s_allTypes, 4 );

	for( auto it = m_batches.begin(); it != m_batches.end(); ++it )
	{
		it->second->Finalize();
	}

	for( unsigned i = 0; i != 4; ++i )
	{
		if( m_typeEnabled[i] )
		{
			renderContext.m_esm.ApplyStandardStates( s_renderingModes[i] );
			renderContext.RenderBatchesWithOverride( m_batches[s_allTypes[i]], m_effectOverride );
		}
	}

	for( auto it = m_batches.begin(); it != m_batches.end(); ++it )
	{
		it->second->Clear();
	}

	return RS_OK;
}

void TriStepRenderObject::py__init__( ITr2Renderable* obj )
{
	m_renderable = obj;
}
