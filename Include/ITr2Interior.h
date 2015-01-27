#pragma once
#ifndef ITr2Interior_H
#define ITr2Interior_H

#include "include/ITr2BoundingBox.h"
#include "ITr2Renderable.h"
#include "Tr2PerObjectData.h"
#include "Resources/TriTextureRes.h"
#include "Tr2AtlasTexture.h"

struct WodStencilBatchParams;
class TriFrustum;
class Tr2InteriorCell;
class Tr2InteriorLightSet;
struct Tr2InteriorPerObjectLightData;
struct Tr2PerFrameVSData;
struct Tr2PerFrameShadowPSData;
struct ITr2InteriorLight;
struct ITr2InteriorSHLightingSolver;
class Tr2RenderContext;
class Tr2InteriorMirror;
struct AxisAlignedBoundingBox;

enum CullResult
{
	CULLRES_OK = 1,
	CULLRES_NOTREADY,
	CULLRES_FAILED,
};

BLUE_DECLARE( Tr2ApexScene );

BLUE_INTERFACE( ITr2InteriorCullable ) : public IRoot
{
	virtual bool IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const = 0;
	virtual size_t GetMirrorCount() const { return 0; }
	virtual Tr2InteriorMirror* GetMirror( size_t index ) const { return nullptr; }
};

BLUE_INTERFACE( ITr2Interior ) : public ITr2InteriorCullable
{
	// Per-object data for instanced lighting
	virtual Tr2PerObjectData* GetPerObjectDataWithPerInstanceLighting( 
		ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet, 
		const Matrix& objectToWorldMatrix, 
		const Matrix& mirrorToWorldMatrix 
	) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Assigns an SH lighting solver to an interior object. Interior
	//   scene assigns a solver right before quering for per object
	//   dataand gathering batches for an object in prepass forward or 
	//   full forward passes. Having a pointer to SH lighting solver
	//   allows an object to register transparent areas that are supposed
	//   to receive SH lighting with the solver. The pointer is guaranteed
	//   to stay alive during next call to GetBatches.
	// Arguments:
	//   solver - Pointer to the solver object
	// -------------------------------------------------------------
	virtual void SetSHLightingSolver( ITr2InteriorSHLightingSolver* solver ) = 0;
};
BLUE_DECLARE_IVECTOR( ITr2Interior );

BLUE_INTERFACE( ITr2InteriorDynamic ) : public ITr2Interior
{
	virtual bool GetWorldBoundingBox( Vector3& min, Vector3& max ) const = 0;
	virtual bool IsBoundingBoxReady( void ) const = 0;
	virtual bool GetShProbePosition( Vector3& position ) const = 0;

	// Spherical harmonics update
	virtual void PrePhysicsUpdate( Be::Time time ) = 0;
	virtual void PostPhysicsUpdate( Be::Time time, Tr2ApexScene* apexScene ) = 0;
	virtual Matrix& GetRedLightProbeMatrix( void ) = 0;
	virtual Matrix& GetGreenLightProbeMatrix( void ) = 0;
	virtual Matrix& GetBlueLightProbeMatrix( void ) = 0;

	// Scene add/remove
	virtual bool AddToScene( Tr2ApexScene* apexScene ) = 0;
	virtual void RemoveFromScene( void ) = 0;

	virtual bool TestCellIntersectionAndAdd( Tr2InteriorCell* cell ) = 0;
	virtual bool IsDirty( void ) const = 0;
	// Set the dirty flag
	virtual void SetDirtyFlag( bool isDirty ) = 0;
	virtual bool IsShadowCaster( void ) const = 0;

	// Some 'dynamic' placeables are actually placed by level designers and do not move or change
	// We allow these to be treated as statics from the point of view of shadows etc
	virtual bool IsStatic( void ) const { return false; }

	// LOD
	virtual void SetLOD( const TriFrustum* frustum ) = 0;
};
BLUE_DECLARE_IVECTOR( ITr2InteriorDynamic );

class TriTextureRes;
class Tr2InteriorDusterCache;

BLUE_DECLARE( Tr2AtlasTexture );

// -------------------------------------------------------------
// Description:
//   ITr2InteriorLight represents light source for interior 
//   scene.
// -------------------------------------------------------------
BLUE_INTERFACE( ITr2InteriorLight ) : public ITr2InteriorCullable
{
	// Type of objects that get rendered into light's shadow map
	enum ShadowCasterTypes
	{
		// No shadow
		ST_NONE				= 0,
		// Generate shadows from statics only
		ST_STATICS_ONLY		= 1,
		// Generate shadows from dynamics only
		ST_DYNAMICS_ONLY	= 2,
		// Generate shadows from statics and dynamics
		ST_ALL				= 3,
	};

	// Helper structure for determining shadow caster importance
	// To Do: Rename this guy & maybe unify it with the LightInstance structure
	struct LightSourceItem
	{
		// Pointer to the light source
		ITr2InteriorLight* lightSource;
		// Light importance
		float importance;
		// Shadow map index
		unsigned int shadowMapIndex;

		// Compare light importance for descending sort
		bool operator<( const LightSourceItem& other ) const
		{
			// for equal importance, sort by lightSource.  this way moving the
			// camera around a pair of lights keeps them in the same order.
			// Which is nice for expensive fx that do something with light0 only.
			if( importance > other.importance )
			{
				return true;
			}
			if( importance < other.importance )
			{
				return false;
			}
			return (size_t)lightSource < (size_t)other.lightSource;
		}
	};

	// -------------------------------------------------------------
	// Description:
	//   Returns axis aligned bounding box for light source in 
	//   world space.
	// -------------------------------------------------------------
	virtual const AxisAlignedBoundingBox& GetBoundingBox() const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns if the light is dirty (moved/changed).
	// Return Value:
	//   true If light is dirty
	//   false Otherwise
	// -------------------------------------------------------------
	virtual bool IsDirty( void ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Set the dirty flag
	// Arguments:
	//   isDirty - If light is dirty (has changed)
	// -------------------------------------------------------------
	virtual void SetDirtyFlag( bool isDirty ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Perform cell intersection test and add light to the cell.
	// Arguments:
	//   cell - Cell to add light to
	// -------------------------------------------------------------
	virtual bool TestCellIntersectionAndAdd( Tr2InteriorCell* cell ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Called whenever light is added to interior scene.
	// -------------------------------------------------------------
	virtual void AddToScene( void ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Called whenever light is removed from interior scene.
	// -------------------------------------------------------------
	virtual void RemoveFromScene( void ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Estimate light's shadow map imoprtance.
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	//   viewerPos - Camera position in world space
	// Return Value:
	//   Value indicating light's shadow "importance": higher values 
	//	 indicate more important shadow sources that are updated more
	//   frequently or assigned to objects in forward pass.
	// -------------------------------------------------------------
	virtual float GetCurrentShadowImportance( unsigned int shadowMapIndex, const Vector3& viewerPos ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Get batches for a light.
	// Arguments:
	//   batches - Batch accumulator to add batches to.
	//   mirrorToWorldMatrix - Mirror to world space transform.
	// -------------------------------------------------------------
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, 
							 const Matrix& mirrorToWorldMatrix ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Copy the light parameters into the per-object data.
	// Arguments:
	//   lightData - Per-object light data
	//   mirrorToWorldMatrix - Mirror to world space transform.
	// -------------------------------------------------------------
	virtual void PopulateLightData( Tr2InteriorPerObjectLightData* lightData, 
									const Matrix &mirrorToWorldMatrix ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns the number of frames since the shadow was updated 
	//   for lights. This method is only called for light sources
	//   that manage their own shadow maps.
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	// Return Value:
	//   Number of frames since the shadow was updated
	// -------------------------------------------------------------
	virtual unsigned int GetFramesSinceShadowUpdate( unsigned int shadowMapIndex ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Sets the number of frames since the shadow was updated for 
	//   lights. This method is only called for light sources
	//   that manage their own shadow maps.
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	//   framesSinceShadowUpdate - Number of frames since the shadow was updated
	// -------------------------------------------------------------
	virtual void SetFramesSinceShadowUpdate( unsigned int shadowMapIndex, unsigned int framesSinceShadowUpdate ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns the number of shadow map textures in shadow atlas
	//   required for this light source.
	// Return Value:
	//   Number of shadow map textures in shadow atlas required
	// -------------------------------------------------------------
	virtual unsigned int GetRequiredShadowMapCount() const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Gives a change to cache shadow map resolution values for a 
	//   light source. Is called every frame before any calls to 
	//   GetRequiredShadowMapResolution.
	// -------------------------------------------------------------
	virtual void CacheShadowMapResolution() = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns dimensions of shadow map required for this light.
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	//   maximumResolution - If true always return light maximum 
	//						 shadow resolution, otherwise can apply 
	//						 LOD scheme.
	//   width (out) - Width of shadow map
	//   height (out) - Height of shadow map
	// -------------------------------------------------------------
	virtual void GetRequiredShadowMapResolution( unsigned int shadowMapIndex, bool maximumResolution, unsigned int& width, unsigned int& height ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns atlas texture containing the given shadow map. Can
	//   return NULL if the texture is not required of was not yet
	//   assigned.
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	// Return Value:
	//   Atlas texture containing the shadow map
	// -------------------------------------------------------------
	virtual Tr2AtlasTexture* GetShadowAtlasTexture( unsigned int shadowMapIndex ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Assigns atlas texture for the given shadow map. 
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	//   texture - Atlas texture containing the shadow map
	// -------------------------------------------------------------
	virtual void SetShadowAtlasTexture( unsigned int shadowMapIndex, Tr2AtlasTexture* texture ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Set shadow empty flag. The interior scene checks if the 
	//   shadow map frustum has any shadow geometry to render and
	//   if it doesn't then the scene removes this shadow map from
	//   atlas and marks it as empty. 
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	//   emptyShadow Flag indicating that the shadow map is empty.
	// -------------------------------------------------------------
	virtual void SetEmptyShadow( unsigned int shadowMapIndex, bool emptyShadow ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns a flag indicating if the shadow map is empty 
	//   (shadow frustum does not contain any shadow geometry to 
	//   render).
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	// Return Value:
	//   true If the shadow map is empty
	//   false Otherwise
	// -------------------------------------------------------------
	virtual bool IsShadowEmpty( unsigned int shadowMapIndex ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Sets up device for shadow map rendering, fills per-frame 
	//   data. This method is only called for light sources that 
	//   manage their own shadow maps.
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	//   vsData (out) - Per-frame VS data to be filled.
	//   psData (out) - Per-frame PS data to be filled.
	// -------------------------------------------------------------
	virtual bool BeginShadowUpdate( unsigned int shadowMapIndex, Tr2PerFrameVSData* vsData, 
		Tr2PerFrameShadowPSData* psData, Tr2RenderContext& renderContext ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Restores device settings after shadow map rendering.
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	// -------------------------------------------------------------
	virtual void EndShadowUpdate( unsigned int shadowMapIndex ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns view matrix for shadow map.
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	// Return Value:
	//   World to shadow map view space transform.
	// -------------------------------------------------------------
	virtual const Matrix& GetViewMatrix( unsigned int shadowMapIndex ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns projection transform matrix for shadow map.
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	// Return Value:
	//   Shadow map view to projection space transform.
	// -------------------------------------------------------------
	virtual const Matrix& GetProjectionMatrix( unsigned int shadowMapIndex ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Marks light's shadow maps that are affected by given bounding
	//   box as dirty. The bounding box usually represents moved dynamic
	//   object.
	// Arguments:
	//   minBounds - Min bounds of a box in world space
	//   maxBounds - Max bounds of a box in world space
	// -------------------------------------------------------------
	virtual void MarkShadowsDirtyForBounds( const Vector3 &minBounds, const Vector3 &maxBounds ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Marks shadow map as dirty. Dirty shadow maps are valid, but
	//   out of date and need to be updated by the scene.
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	//   dirty - Flag indicating that the shadow map is dirty
	// -------------------------------------------------------------
	virtual void MarkShadowDirty( unsigned int shadowMapIndex, bool dirty ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns shadow map dirty flag. Dirty shadow maps are valid, but
	//   out of date and need to be updated by the scene.
	// Arguments:
	//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
	// Return Value:
	//   true If light's shadow map is dirty and need updating
	//	 false Otherwise
	// -------------------------------------------------------------
	virtual bool IsShadowDirty( unsigned int shadowMapIndex ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns types of objects that cast shadows from this light.
	//   out of date and need to be updated by the scene.
	// Return Value:
	//   Types of objects that cast shadows from this light
	// -------------------------------------------------------------
	virtual ShadowCasterTypes GetShadowCasterTypes() const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Get batches for a light to use during SH lighting step.
	// Arguments:
	//   batches - Batch accumulator to add batches to.
	// -------------------------------------------------------------
	virtual void GetSHBatches( ITriRenderBatchAccumulator* batches ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Per-frame update method.
	// Arguments:
	//   time - Current system time.
	// -------------------------------------------------------------
	virtual void Update( Be::Time time ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   This function centralizes some more complicated logic for if a potentially dynamic object
	//   casts shadows. Many dynamic objects can be marked as 'static' if they're placed by level designers
	//   and are not animated in these cases (tables etc) we want them to be included in the static shadow maps
	//	 which we generate and try not to update.
	//	 NB: Moving these dynamics will cause terrible performance!
	// Arguments:
	//   dynamic - The dynamic in question
	// -------------------------------------------------------------
	bool IsDynamicContributingToShadows( ITr2InteriorDynamic const* dynamic ) const
	{
		if( !dynamic->IsShadowCaster() )
		{
			return false;
		}

		ShadowCasterTypes shadowTypes = GetShadowCasterTypes();

		if( shadowTypes == ST_ALL )
		{
			return true;
		}
		else if( shadowTypes == ST_NONE )
		{
			return false;
		}

		const bool dynamicIsStatic = dynamic->IsStatic();

		if( dynamicIsStatic && (shadowTypes & ST_STATICS_ONLY) )
		{
			return true;
		}
		else if( !dynamicIsStatic && (shadowTypes & ST_DYNAMICS_ONLY) )
		{
			return true;
		}

		return false;
	}

};

BLUE_DECLARE_IVECTOR( ITr2InteriorLight );

class Tr2PerObjectDataPSBuffer;

// -------------------------------------------------------------
// Description:
//   ITr2InteriorSHLightingSolver is an interface for SH lighting
//   solver. A solver is used to gather per-area data for transparent
//   areas that want to receive SH lighting and to compute SH 
//   coefficients for these areas.
// -------------------------------------------------------------
struct ITr2InteriorSHLightingSolver
{
	// -------------------------------------------------------------
	// Description:
	//   Adds a volume representing a transparent area that nees
	//   SH coefficients computed. The result of computation is stored
	//   in provided per-area data.
	// Arguments:
	//   min  - Min bounds of the area in local space
	//   max  - Max bounds of the area in local space
	//   transform  - Transform matrix from local to world space
	//   perAreaData - Per-area data block that is used to store the
	//                 result of computation. This pointer needs to
	//                 valid until ITr2InteriorSHLightingSolver::Solve
	//                 is called.
	// -------------------------------------------------------------
	virtual void AddVolume( const Vector3& min, const Vector3& max, const Matrix& transform, Tr2PerObjectDataPSBuffer* perAreaData ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Performs SH lighting computation for stored areas usign light
	//   set provided.
	// Arguments:
	//   visibleLights  - A set of visible light sources
	//   renderContext - Current render context
	// -------------------------------------------------------------
	virtual void Solve( const ITr2InteriorLightVector& visibleLights, Tr2RenderContext& renderContext ) = 0;
};

// -------------------------------------------------------------
// Description:
//   PerObjectData helper that avoids copy/paste for setting a projected
//   light into slot 11 for every type of light that we have
// -------------------------------------------------------------
class Tr2ProjectedPerLightData : public Tr2PerObjectDataStandard
{
public:
	void SetProjectedTexture( TriTextureRes* textureRes )
	{
		if( textureRes && textureRes->IsGood() )
		{
			m_projectedTexture = textureRes->GetTexture();
		}
		else
		{
			m_projectedTexture = nullptr;
		}
	}
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
	{
		Tr2PerObjectDataStandard::SetPerObjectDataToDevice( buffers, constantTypeMask, renderContext );

		if( m_projectedTexture )
		{
			renderContext.SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 11, *m_projectedTexture );
		}
		else
		{
			renderContext.SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 11, nullTX );
		}
	}
private:
	// These are only held temporarily within the render context
	// There is no destructor call for per-object data, 
	// so it is not possible to reference count these correctly in this class
	Tr2TextureAL* m_projectedTexture;
};

template<typename BaseClass>
class Tr2ShadowPerLightData : public BaseClass
{
public:
	void SetShadowTexture( Tr2AtlasTexture* atlasTexture )
	{
		if( atlasTexture )
		{
			//m_shadowTexture = textureRes->GetTexture();
			m_shadowTexture = atlasTexture->GetTexture();
		}
		else
		{
			m_shadowTexture = nullptr;
		}
	}
	void SetShadowTexture( const Tr2AtlasTexturePtr* atlasTextures, unsigned count )
	{
		for( unsigned i = 0; i != count; ++i )
		{
			if( atlasTextures[i] )
			{
				SetShadowTexture( atlasTextures[i] );
				return;
			}
		}

		m_shadowTexture = nullptr;
	}
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
	{
		BaseClass::SetPerObjectDataToDevice( buffers, constantTypeMask, renderContext );

		if( m_shadowTexture )
		{
			renderContext.SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 10, *m_shadowTexture );
		}
		else
		{
			renderContext.SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 10, nullTX );
		}
	}
private:
	// These are only held temporarily within the render context
	// There is no destructor call for per-object data, 
	// so it is not possible to reference count these correctly in this class
	Tr2TextureAL* m_shadowTexture;
};
#endif // ITr2Interior_H
