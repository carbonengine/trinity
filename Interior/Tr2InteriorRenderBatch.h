#pragma once

#ifndef Tr2InteriorRenderBatch_H
#define Tr2InteriorRenderBatch_H

#include "TriRenderBatch.h"
#include "Tr2ConstantBufferFormats.h"

// --------------------------------------------------------------------------------------
// Description
//   This is a custom derivation of TriClippingBatch for WoD interior rendering.  In 
//   addition to setting a user clip plane, this class also provides a mechanism for 
//   inverting the cull mode when crossing a mirror.  That is, if the normal cull mode 
//   is CCW, inversion will set the cull mode to CW (and vice versa).  It maintains a 
//   copy of the per-frame data, so that it can pass a float to the shaders indicating the 
//   current cull direction.  This is needed for correct normal mapping in mirrored space.
// See Also
//   TriRenderBatch, TriClippingBatch
// Summary
//   Adds cull mode inversion along with user clipping planes for WoD interior rendering 
//   (used for mirrors)                                      
// --------------------------------------------------------------------------------------
class Tr2InteriorClippingBatch : public TriClippingBatch
{
public:
	// Destructor
	virtual ~Tr2InteriorClippingBatch() {}

	// Sets the local copy of the per-frame data
	void SetPerFramePSData( Tr2PerFramePSData& psData )
	{
		memcpy( &m_psData, &psData, sizeof(Tr2PerFramePSData) );
	}

	// Sets cull mode inversion
	virtual void SetInvertedCullMode( bool invert )
	{
		m_isCullModeInverted = invert;
		m_psData.cullDirection = m_isCullModeInverted ? -1.0f : 1.0f;
	}

	// Sets the per-frame data, cull mode, and user clip plane
	virtual void SubmitGeometry( Tr2RenderContext& renderContext );

	// For debugging in PIX
	virtual const std::string& GetBatchTypeName( void ) const
	{ 
		return Tr2InteriorClippingBatch::s_batchTypeName; 
	}
	static const std::string s_batchTypeName;

private:
	// Local copy of the per-frame data
	Tr2PerFramePSData m_psData;

	Tr2ConstantBufferAL	m_PSBuffer;
};

struct WodStencilBatchParams
{
	int m_meshIx;
	int m_areaIx;
	int m_stencilWrite;
	int m_stencilTest;
	uint32_t m_stencilPassState;
	bool m_depthClear;
	bool m_colorWrite;
};

// --------------------------------------------------------------------------------------
// Description
//   This is a custom derivation of TriGeometryBatch for drawing a stencil mask and 
//   manipulating the depth buffer.  Mirrors use this to mask out the mirror face and 
//   clear the depth buffer in the mirrored region.
// See Also
//   TriRenderBatch, TriGeometryBatch
// Summary
//   Allows custom stencil and depth buffer rendering for masking out mirror portals
// --------------------------------------------------------------------------------------
class Tr2InteriorStencilMaskBatch : public TriGeometryBatch
{
public:
	// Destructor
	virtual ~Tr2InteriorStencilMaskBatch() {}

	// Sets depth/stencil state and draws mask geometry
	virtual void SubmitGeometry( Tr2RenderContext& renderContext );

	// Set the stencil test and stencil write values
	void SetStencilValues( int write, int test )
	{
		m_stencilWrite = write;
		m_stencilTest = test;
		m_setStencilTestOnly = false;
	}

	// Do depth clear?
	void SetDepthClear( bool clear ) { m_doDepthClear = clear; }
	// Stencil pass state
	void SetStencilPassState( uint32_t state ) { m_stencilPassState = state; }
	// Disable stencil
	void SetDisableStencil( bool disable) { m_disableStencil = disable; }
	// Set stencil test value
	void SetStencilTest( int stencilTest )
	{
		m_stencilTest = stencilTest;
		m_setStencilTestOnly = true;
	}
	// Set if color writes are enabled
	void SetColorWrite( bool colorWrite )
	{
		m_colorWrite = colorWrite;
	}
	
	// For debugging in PIX
	virtual const std::string& GetBatchTypeName( void ) const
	{ 
		return Tr2InteriorStencilMaskBatch::s_batchTypeName; 
	}
	static const std::string s_batchTypeName;

private:
	int m_stencilWrite;
	int m_stencilTest;
	bool m_doDepthClear;
	bool m_setStencilTestOnly;
	uint32_t m_stencilPassState;
	bool m_disableStencil;
	bool m_colorWrite;
};

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
	virtual bool RenderWithOverride( void ) const { return false; }

	// For debugging in PIX
	virtual const std::string& GetBatchTypeName( void ) const
	{ 
		return Tr2InteriorBackgroundCubemapBatch::s_batchTypeName; 
	}
	static const std::string s_batchTypeName;
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

		// Opaque batches are sort-by-effect
		if( group == (unsigned int)WODINTBATCHGROUP_OPAQUE )
		{
			// Get the effect sort key
			int64_t effectKey = 0x00000000FFFFFFFF;
			ITr2ShaderMaterial* shaderMaterial = entry.m_batch->GetShaderMaterialInterface();
			if( shaderMaterial )
			{
				effectKey = (int64_t)shaderMaterial->GetSortValue();
			}

			// Pack the effect sort key into the low 32 bits
			entry.m_sortKey = baseKey | effectKey;
		}
		// Everything else is sort-by-depth
		else
		{
			// Note: the depth key is set to zero because Umbra gives objects in front-to-back order,
			// thus allowing us to pre-sort batches in front-to-back order (for decals) or
			// back-to-front (for alpha-blend).
			int64_t depthKey = entry.m_batch->GetDepth();

			// Pack the depth key into the low 32 bits
			entry.m_sortKey = baseKey | depthKey;
		}
	}

	// Get the sort type - need stable_sort so decals stay in artist-specified order
	RenderBatchSortType GetSortType( void ) const 
	{ 
		return RENDERBATCHSORTTYPE_STABLE_SORT; 
	}
};

#endif