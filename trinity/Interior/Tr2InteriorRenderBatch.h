#pragma once

#ifndef Tr2InteriorRenderBatch_H
#define Tr2InteriorRenderBatch_H

#include "TriRenderBatch.h"
#include "Tr2ConstantBufferFormats.h"


// --------------------------------------------------------------------------------------
// Description
//   This is a custom derivation of TriGeometryBatch for drawing the background cubemap 
//   inline with the normal stream of render batches.  Needed to ensure that the 
//   background is reflected correctly.
// See Also
//   TriRenderBatch, TriGeometryBatch
// Summary
//   Draws a cubemap as the background.
// --------------------------------------------------------------------------------------
class Tr2InteriorBackgroundCubemapBatch : public TriGeometryBatch
{
public:
	// Destructor
	virtual ~Tr2InteriorBackgroundCubemapBatch() {}

	// Draws the background cubemap using the specified effect
	virtual void SubmitGeometry( Tr2RenderContext& renderContext );
	// Don't render with override effects
	virtual OverrideOptions RenderWithOverride( void ) const { return DO_NOT_RENDER_WITH_OVERRIDE; }

	// For debugging in PIX
	virtual const std::string& GetBatchTypeName( void ) const
	{ 
		static const std::string name = "Tr2InteriorBackgroundCubemapBatch";
		return name; 
	}
};

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
	// Generate a sort key for the interior render batch
	void GenerateKey( RenderBatchSortEntry& entry ) const
	{
		const int64_t mask = 0x0000FFFF00000000;
		const int64_t baseKey = entry.m_batch->GetUserData();
		int64_t group = ( baseKey & mask ) >> 32;
		const int64_t userDataMask = group << 56;

		// Opaque batches are sort-by-effect
		if( group == (unsigned int)WODINTBATCHGROUP_OPAQUE )
		{
			// Get the effect sort key
			int64_t effectKey = 0x00FFFFFFFFFFFFFF;
			auto shaderMaterial = entry.m_batch->GetShaderMaterialInterface();
			if( shaderMaterial )
			{
				effectKey = shaderMaterial->GetSortValue() & effectKey;
			}

			// Pack the effect sort key into the low 32 bits
			entry.m_sortKey = userDataMask | effectKey;
		}
		// Everything else is sort-by-depth
		else
		{
			// Note: the depth key is set to zero because Umbra gives objects in front-to-back order,
			// thus allowing us to pre-sort batches in front-to-back order (for decals) or
			// back-to-front (for alpha-blend).
			int64_t depthKey = entry.m_batch->GetDepth();

			// Pack the depth key into the low 32 bits
			entry.m_sortKey = userDataMask | depthKey;
		}
	}

	// Get the sort type - need stable_sort so decals stay in artist-specified order
	RenderBatchSortType GetSortType( void ) const 
	{ 
		return RENDERBATCHSORTTYPE_STABLE_SORT; 
	}
};

#endif