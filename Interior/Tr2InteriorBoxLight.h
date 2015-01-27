////////////////////////////////////////////////////////////
//
//    Created:   July 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef Tr2InteriorBoxLight_H
#define Tr2InteriorBoxLight_H

#include "include/ITr2Interior.h"
#include "Tr2DeviceResource.h"
#include "Tr2InteriorConstantBufferFormats.h"
#include "Utilities/BoundingBox.h"

// --------------------------------------------------------------------------------------
// Blue forwards
BLUE_DECLARE( Tr2InteriorCell );
BLUE_DECLARE_VECTOR( Tr2InteriorCell );
BLUE_DECLARE( Tr2InteriorLightSource );
BLUE_DECLARE( Tr2ShaderMaterial );
BLUE_DECLARE( Tr2KelvinColor );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE_INTERFACE( ITr2InteriorDynamic );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
class ITriRenderBatchAccumulator;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorBoxLight represents an interior light, that is a directional light bounded
//   by a object-oriented box. Lights are managed at the Tr2InteriorScene level. 
// See Also:
//   Tr2InteriorScene, Tr2InteriorLightSet
// --------------------------------------------------------------------------------------
class Tr2InteriorBoxLight :
	public INotify,
	public IInitialize,
	public Tr2DeviceResource,
	public ITr2InteriorLight,
	public IBlueAsyncResNotifyTarget
{
public:
	// Constructor
	Tr2InteriorBoxLight( IRoot* lockobj = 0 );
	// Destructor
	~Tr2InteriorBoxLight();

	EXPOSE_TO_BLUE();

    using IInitialize::Lock;
    using IInitialize::Unlock;

	/////////////////////////////////////////////////////////////
	// IInitialize
    bool Initialize();

	/////////////////////////////////////////////////////////////
	// INotify
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
	void PopulateLightData( Tr2InteriorPerObjectLightData* lightData, const Matrix& mirrorToWorldMatrix ) const;

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

	// Get batches for an instanced light
	void GetBatches( ITriRenderBatchAccumulator* batches, const Matrix& mirrorToWorldMatrix );
	// Get batches for a light to use during SH lighting step
	void GetSHBatches( ITriRenderBatchAccumulator* batches ) const;

	// Returns the number of frames since the shadow was updated for spot lights
	unsigned int GetFramesSinceShadowUpdate( unsigned int shadowMapIndex ) const { return m_framesSinceShadowUpdate; }
	// Sets the number of frames since the shadow was updated for spot lights
	void SetFramesSinceShadowUpdate( unsigned int shadowMapIndex, unsigned int framesSinceShadowUpdate ) { m_framesSinceShadowUpdate = framesSinceShadowUpdate; }

	unsigned int GetRequiredShadowMapCount() const;
	void CacheShadowMapResolution();
	void GetRequiredShadowMapResolution( unsigned int index, bool maximumResolution, unsigned int& width, unsigned int& height ) const;
	Tr2AtlasTexture* GetShadowAtlasTexture( unsigned int index ) const;
	void SetShadowAtlasTexture( unsigned int index, Tr2AtlasTexture* texture );

	// Set flag if the shadow map is empty
	void SetEmptyShadow( unsigned int shadowMapIndex, bool emptyShadow );
	// Returns a flag if the shadow map is empty
	bool IsShadowEmpty( unsigned int shadowMapIndex ) const;

	// Setup device, fill per-frame data for shadow map rendering
	bool BeginShadowUpdate( unsigned int shadowMapIndex, Tr2PerFrameVSData* vsData, Tr2PerFrameShadowPSData* psData, Tr2RenderContext& renderContext );
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

	// Position getter & setter
	const Vector3& GetPosition( void ) const { return *(reinterpret_cast<const Vector3*>(&m_transform._41)); }
	void SetPosition( const Vector3& pos );

	// Rotation getter & setter
	Quaternion GetRotation( void ) const;
	void SetRotation( const Quaternion& rotQuat );

	// Scaling getter & setter
	Vector3 GetScaling( void ) const;
	void SetScaling( const Vector3& scaleVec );

	void Update( Be::Time time );
protected:
	// Rebuild bounding volume
	void RebuildVolume( void );

	// Called when light transform is modified
	void TransformModified();
private:
	AxisAlignedBoundingBox GetBoundingBoxInLocalSpace() const;
	// Chooses an appropriate effect for light rendering
	void ChooseLightEffect();
	// Re-creates shadow map
	void CreateShadowMap();

	// Light names
	std::string m_name;

	// Light unit to world transform
	Matrix m_transform;
	AxisAlignedBoundingBox m_worldBoundingBox;
	// Light diffuse Color
	Color m_color;
	// Falloff over distance
	float m_falloff;
	// Specular intensity
	float m_specularIntensity;
	// Shadow casting importance
	float m_shadowImportance;

	// Kelvin color temperature
	Tr2KelvinColorPtr m_kelvinColor;
	// Toggle between rgb and kelvin
	bool m_useKelvinColor;

	// Does this light contribute to direct lighting?
	bool m_primaryLighting;
	// Does this light affect transparent objects?
	bool m_affectTransparentObjects;

	// Does this light source require an update?
	bool m_isDirty;

	// Matrix representation of the oriented bounding box (for spot lights)
	Matrix m_obbMatrix;

	// Box center for xnamath OBB representation
	Vector3 m_collisionCenter;
	// Box extents for xnamath OBB representation 
	Vector3 m_collisionExtents;
	// Orientation quat for xnamath OBB representation
	Quaternion m_collisionOrientation;

	// Vertex declaration for light geometry (for light accumulation pass)
	unsigned int m_lightVertexDecl;

	// Shader material for box lights
	Tr2ShaderMaterialPtr m_material;
	// Shader material for SH lighting pass
	Tr2ShaderMaterialPtr m_shMaterial;

	// Shadow texture for spot lights
	Tr2AtlasTexturePtr m_shadowTexture;
	// Shadow texture resolution for spot lights
	unsigned int m_shadowResolution;
	// Cached shadow map resolution to the current frame
	unsigned int m_currentShadowResolution;
	// Enable shadow resolution LODs
	bool m_enableShadowLOD;
	// Number of frames the shadow texture was not updated
	unsigned int m_framesSinceShadowUpdate;
	// Flag indicating that light's shadow is updated
	bool m_emptyShadow;
	// Light view matrix
	Matrix m_viewMatrix;
	// Light projection matrix
	Matrix m_projectionMatrix;
	// Flag indicating that the shadow is dirty and needs updating
	bool m_dirtyShadow;
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

TYPEDEF_BLUECLASS( Tr2InteriorBoxLight );
BLUE_DECLARE_VECTOR( Tr2InteriorBoxLight );


#endif // Tr2InteriorBoxLight_H