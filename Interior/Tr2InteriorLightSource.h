#pragma once
#ifndef Tr2InteriorLightSource_H
#define Tr2InteriorLightSource_H

#include "include/ITr2Interior.h"
#include "Tr2DeviceResource.h"
#include "TriFrustum.h"
#include "Utilities/BoundingBox.h"

// --------------------------------------------------------------------------------------
// Blue forwards
BLUE_DECLARE( Tr2InteriorCell );
BLUE_DECLARE_VECTOR( Tr2InteriorCell );
BLUE_DECLARE( Tr2InteriorLightSource );
BLUE_DECLARE( Tr2ShaderMaterial );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2KelvinColor );
BLUE_DECLARE_INTERFACE( ITr2InteriorDynamic );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
class ITriRenderBatchAccumulator;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorLightSource represents an interior light, which can behave as either a 
//   point or spot light, depending on parameters.  Lights are managed at the 
//   Tr2InteriorScene level.
// See Also:
//   Tr2InteriorScene, Tr2InteriorLightSet
// --------------------------------------------------------------------------------------
class Tr2InteriorLightSource :
	public INotify,
	public IInitialize,
	public Tr2DeviceResource,
	public ITr2InteriorLight,
	public IBlueAsyncResNotifyTarget
{
public:
	// Constructor
	Tr2InteriorLightSource( IRoot* lockobj = 0 );
	// Destructor
	~Tr2InteriorLightSource();

	EXPOSE_TO_BLUE();

    using IInitialize::Lock;
    using IInitialize::Unlock;

	// Initialization callback
    bool Initialize();

	// Value-modified callback
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:	

	/////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* p ) {}
	void RebuildCachedData( BlueAsyncRes* p );

    //////////////////////////////////////////////////////////////////////////
    // ITr2InteriorCullable
	virtual bool IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const;

	/////////////////////////////////////////////////////////////
	// ITr2InteriorLight

	// Copy the light parameters into the per-object data
	void PopulateLightData( Tr2InteriorPerObjectLightData* lightData, 
		const Matrix &mirrorToWorldMatrix ) const;

	// Get importance scale as applied to the View importance of this light.
	float GetImportanceScale()		const { return m_importanceScale; }
	// Get importance bias as applied to the View importance of this light.
	float GetImportanceBias()		const { return m_importanceBias; }


	// Get the axis aligned bounding box
	const AxisAlignedBoundingBox& GetBoundingBox() const;

	// Is the light dirty (needs cell-intersection update)
	bool IsDirty( void ) const { return m_isDirty; }
	// Set the dirty flag
	void SetDirtyFlag( bool isDirty ) { m_isDirty = isDirty; }

	// Cell intersection
	bool TestCellIntersectionAndAdd( Tr2InteriorCell* cell );

	// Determine overall scene influence
	float GetCurrentViewImportance( const Vector3& viewerPos ) const;
	// Determine overall scene influence
	float GetCurrentShadowImportance( unsigned int shadowMapIndex, const Vector3& viewerPos ) const;

	// Add the light to the scene
	void AddToScene( void );
	// Remove the light from the scene
	void RemoveFromScene( void );

	// Returns true if the cell is in the manual cell list used for light-cell 
	// intersection test (when CellIntersectionType is IT_MANUAL)
	bool HasCellInManualList( const Tr2InteriorCell* cell ) const;
	// Adds a cell to manual cell list used for light-cell intersection test 
	// (when CellIntersectionType is IT_MANUAL)
	void AddCellToManualList( Tr2InteriorCell* cell );

	// Get batches for an instanced light
	void GetBatches( ITriRenderBatchAccumulator* batches, const Matrix& mirrorToWorldMatrix );
	// Get batches for a light to use during SH lighting step
	void GetSHBatches( ITriRenderBatchAccumulator* batches ) const;

	// Returns the number of frames since the shadow was updated for spot lights
	unsigned int GetFramesSinceShadowUpdate( unsigned int shadowMapIndex ) const;
	// Sets the number of frames since the shadow was updated for spot lights
	void SetFramesSinceShadowUpdate( unsigned int shadowMapIndex, unsigned int framesSinceShadowUpdate );

	// Set flag if the shadow map is empty
	void SetEmptyShadow( unsigned int shadowMapIndex, bool emptyShadow );
	// Returns a flag if the shadow map is empty
	bool IsShadowEmpty( unsigned int shadowMapIndex ) const;

	unsigned int GetRequiredShadowMapCount() const;
	void CacheShadowMapResolution();
	void GetRequiredShadowMapResolution( unsigned int index, bool maximumResolution, unsigned int& width, unsigned int& height ) const;
	Tr2AtlasTexture* GetShadowAtlasTexture( unsigned int index ) const;
	void SetShadowAtlasTexture( unsigned int index, Tr2AtlasTexture* texture );

	// Setup device, fill per-frame data for shadow map rendering
	bool BeginShadowUpdate( unsigned int shadowMapIndex, Tr2PerFrameVSData* vsData, 
		Tr2PerFrameShadowPSData* psData, Tr2RenderContext& renderContext );
	// Restore device after shadow map rendering
	void EndShadowUpdate( unsigned int shadowMapIndex );
	// Return shadow map view matrix
	const Matrix& GetViewMatrix( unsigned int shadowMapIndex ) const;
	// Return shadow map projection transform matrix
	const Matrix& GetProjectionMatrix( unsigned int shadowMapIndex ) const;

	// Mark shadow dirty if it is affected by bounds
	void MarkShadowsDirtyForBounds( const Vector3 &minBounds, const Vector3 &maxBounds );
	// Mark shadow map as dirty
	void MarkShadowDirty( unsigned int shadowMapIndex, bool dirty );
	// Return if the shadow map is dirty
	bool IsShadowDirty( unsigned int shadowMapIndex ) const;
	// Return shadow caster object types
	ShadowCasterTypes GetShadowCasterTypes() const;

	void Update( Be::Time time );

	void UpdateInternalMaterials();
protected:
	// Rebuild bounding volume
	void RebuildVolume( void );

	// Is this a spotlight?
	bool IsSpotLight()					const { return ( m_coneAlphaOuter < 89.f ); }
protected:
	void RecalculateUnitToWorldMatrix();
	// Chooses an appropriate effect for light rendering
	void ChooseLightEffect();
	// Re-creates shadow map if light source owns a shadow map
	void CreateShadowMap();
	// Updates shadow map frustum
	void UpdateSpotlightFrustum();
	// Rebuilds light geometry
	void RebuildGeometry();

	// Light names
	std::string m_name;

	// Light position
	Vector3 m_position;
	// Light radius
	float m_radius;
	AxisAlignedBoundingBox m_worldBoundingBox;
	// Light diffuse Color
	Color m_color;
	// Falloff over radius
	float m_falloff;
	// Specular intensity
	float m_specularIntensity;
	// Shadow casting importance
	float m_shadowImportance;
	// Outer spotlight angle
	float m_coneAlphaOuter;
	// Inner spotlight angle
	float m_coneAlphaInner;
	// Spotlight direction
	Vector3 m_coneDirection;

	// Kelvin color temperature
	Tr2KelvinColorPtr m_kelvinColor;
	// Toggle between rgb and kelvin
	bool m_useKelvinColor;

	// Does this light contribute to direct lighting?
	bool m_primaryLighting;
	// Does this light affect transparent objects?
	bool m_affectTransparentObjects;

	// A multiplier for importance. In practice actually for turning off sorting, or by
	// enforcing a certain order, by setting it to zero for all lights and tweaking bias instead.
	float m_importanceScale;
	// A bias. Final view importance is [computed] * scale + bias
	float m_importanceBias;

	// Does this light source require an update?
	bool m_isDirty;

	// Box center for xnamath OBB representation
	Vector3 m_collisionCenter;
	// Box extents for xnamath OBB representation 
	Vector3 m_collisionExtents;
	// Orientation quat for xnamath OBB representation
	Quaternion m_collisionOrientation;

	// Vertex declaration for light geometry (for light accumulation pass)
	unsigned int m_lightVertexDecl;
	//// Shadow intensity from 0 to 1
	//float m_shadowIntensity;
	//// Shadow texture
	//TriTextureResPtr m_shadow;

	// Shader material for various light types
	Tr2ShaderMaterialPtr m_material;
	// Shader material for SH lighting pass
	Tr2ShaderMaterialPtr m_shMaterial;

	// Custom light material provided by user
	Tr2ShaderMaterialPtr m_customMaterial;

	// Cached unit to world light geometry transform for light accumulation pass
	Matrix m_unitToWorldTransform;

	// Shadow texture for spot lights if light uses atlas
	Tr2AtlasTexturePtr m_shadowAtlasTexture[6];
	// Shadow texture resolution for spot lights
	unsigned int m_shadowResolution;
	// Cached shadow map resolution to the current frame
	unsigned int m_currentShadowResolution;
	// Enable shadow resolution LODs
	bool m_enableShadowLOD;
	// Number of frames the shadow texture was not updated
	unsigned int m_framesSinceShadowUpdate[6];
	// Flag indicating that light's shadow is updated
	bool m_emptyShadow[6];
	// Spotlight view matrix
	Matrix m_viewMatrix[6];
	// Spotlight projection matrix
	Matrix m_projectionMatrix;
	// Spotlight projection frustum
	TriFrustum m_frustum[6];
	// Flag indicating that the spotlight shadow is dirty and needs updating
	bool m_dirtySpotLightShadow[6];
	// Filter on type of objects rendered into the shadow map
	ShadowCasterTypes m_shadowCasterTypes;

	// Path to light's projected texture
	std::string m_projectedTexturePath;
	// Light's projected texture
	TriTextureResPtr m_projectedTextureRes;

	PTriCurveSetVector m_curveSets;

	// Shared vertex buffer for unclipped lights
	Tr2VertexBufferAL* m_sharedVB;
	// Shared index buffer for unclipped lights
	Tr2IndexBufferAL* m_sharedIB;

	class PerLightData;
};

TYPEDEF_BLUECLASS( Tr2InteriorLightSource );
BLUE_DECLARE_VECTOR( Tr2InteriorLightSource );

#endif // Tr2InteriorLightSource_H
