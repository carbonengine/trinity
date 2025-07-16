#pragma once

#ifndef Tr2InteriorRenderBatch_H
#define Tr2InteriorRenderBatch_H

#include "TriRenderBatch.h"
#include "Tr2ConstantBufferFormats.h"
#include "../Shader/Tr2Shader.h"


// --------------------------------------------------------------------------------------
// Description
//   Enumeration of interior batch groups (secondary sort key for interior batches).
// --------------------------------------------------------------------------------------
enum Tr2InteriorBatchGroup
{
	WODINTBATCHGROUP_BEGIN,		// Batches that must preceed normal geometry batches
	WODINTBATCHGROUP_OPAQUE,	// Opaque batches (sorted by effect)
	WODINTBATCHGROUP_DECAL,		// Decal batches (unsorted, artist-specified order)
	WODINTBATCHGROUP_BLEND,		// Blended batches (sorted by depth)
	WODINTBATCHGROUP_END		// Batches that must follow the normal geometry batches
};

// --------------------------------------------------------------------------------------
// Description
//   Render batch sort key generator for interior render batches.  The batches are sorted
//   using a 64-bit key.  The high 16-bits encode the object group (corresponding to a
//   cell).  The next highest 16-bits encode the batch group within the current object
//   group, using the Tr2InteriorBatchGroup enum.  The low 32-bits encode either the 
//   effect sort key (for opaque batches) or the depth (for blended batches).
// See Also
//   TriRenderBatchAccumulator, DefaultKeyGenerator, EffectKeyGenerator
// Summary
//   Render batch sort key generator for interior render batches.
// --------------------------------------------------------------------------------------
struct Tr2IntKeyGenerator
{
	static bool Less( const Tr2RenderBatch& batch1, const Tr2RenderBatch& batch2 )
	{
		if( batch1.m_renderingMode < batch2.m_renderingMode )
		{
			return true;
		}
		if( batch1.m_renderingMode > batch2.m_renderingMode )
		{
			return false;
		}
		if( batch1.m_renderingMode == Tr2EffectStateManager::RM_ALPHA )
		{
			return batch1.m_depth < batch2.m_depth;
		}
		else
		{
			return 0;
		}
	}

	// Get the sort type - need stable_sort so decals stay in artist-specified order
	constexpr static RenderBatchSortType GetSortType() 
	{ 
		return RENDERBATCHSORTTYPE_STABLE_SORT; 
	}

	static constexpr bool ALLOW_GDPR = false;
};

#endif