////////////////////////////////////////////////////////////
//
//    Created:   October 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef Tr2InteriorCylinderLight_H
#define Tr2InteriorCylinderLight_H

#include "include/ITr2Interior.h"
#include "Tr2DeviceResource.h"
#include "Utilities/BoundingBox.h"

// --------------------------------------------------------------------------------------
// Blue forwards
BLUE_DECLARE( Tr2InteriorCell );
BLUE_DECLARE( Tr2KelvinColor );
BLUE_DECLARE( Tr2ShaderMaterial );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE_INTERFACE( ITr2InteriorDynamic );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
class ITriRenderBatchAccumulator;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorCylinderLight represents an cylinder (tube) interior light. It radiates 
//   light from and perpendicular to its cylinder axis. Lights are managed at the 
//   Tr2InteriorScene level.
// See Also:
//   Tr2InteriorScene, Tr2InteriorLightSet
// --------------------------------------------------------------------------------------
class Tr2InteriorCylinderLight :
	public INotify,
	public IInitialize,
	public Tr2DeviceResource,
	public ITr2InteriorLight,
	public IBlueAsyncResNotifyTarget
{
public:
	// Constructor
	Tr2InteriorCylinderLight( IRoot* lockobj = 0 );
	// Destructor
	~Tr2InteriorCylinderLight();

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
	// Tr2DeviceResource
	virtual void ReleaseResources( TriStorage s );

	/////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	virtual void ReleaseCachedData( BlueAsyncRes* p );
	virtual void RebuildCachedData( BlueAsyncRes* p );

    //////////////////////////////////////////////////////////////////////////
    // ITr2InteriorCullable
	virtual bool IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const;

	/////////////////////////////////////////////////////////////
	// ITr2InteriorLight

	// Copy the light parameters into the per-object data
	void PopulateLightData( Tr2InteriorPerObjectLightData* lightData, const Matrix& mirrorToWorldMatrix ) const;

	// Get the axis aligned bounding box
	const AxisAlignedBoundingBox& GetBoundingBox() const;
	// Get cylinder radius
	float GetRadius() const { return m_radius; }
	// Get cylinder length
	float GetLength() const { return m_length; }

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
	unsigned int GetFramesSinceShadowUpdate( unsigned int shadowMapIndex ) const { return 0; }
	// Sets the number of frames since the shadow was updated for spot lights
	void SetFramesSinceShadowUpdate( unsigned int shadowMapIndex, unsigned int framesSinceShadowUpdate ) { }

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

	void Update( Be::Time time );

	void EnableROI( bool enable );
protected:
	// Rebuild bounding volume
	void RebuildVolume( void );

	// Called when light transform is modified
	void TransformModified();
private:
	AxisAlignedBoundingBox GetBoundingBoxInLocalSpace() const;

	/////////////////////////////////////////////////////////////
	// Tr2DeviceResource
    virtual bool OnPrepareResources();

	// Chooses an appropriate effect for light rendering
	void ChooseLightEffect();

	// Light names
	std::string m_name;

	// Light unit to world space transform
	// Cylinder is aligned along Z axis in its local CS
	Matrix m_transform;
	AxisAlignedBoundingBox m_worldBoundingBox;
	// Cylinder radius
	float m_radius;
	// Cylinder length (height)
	float m_length;
	// Position of center of light
	Vector3 m_position;
	// Rotation of light
	Quaternion m_rotation;

	// Light diffuse Color
	Color m_color;
	// Falloff over radius
	float m_falloff;
	// Specular intensity
	float m_specularIntensity;

	// Cylinder sector angle outer
	float m_sectorAngleOuter;
	// Cylinder sector angle inner
	float m_sectorAngleInner;

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

	// Matrix representation of the oriented bounding box
	Matrix m_obbMatrix;

	// Vertex declaration for light geometry (for light accumulation pass)
	unsigned int m_lightVertexDecl;
	
	// Shader material for box lights
	Tr2ShaderMaterialPtr m_material;
	// Shader material for SH lighting pass
	Tr2ShaderMaterialPtr m_shMaterial;

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
	struct ColorDusters;
};

TYPEDEF_BLUECLASS( Tr2InteriorCylinderLight );
BLUE_DECLARE_VECTOR( Tr2InteriorCylinderLight );


#endif // Tr2InteriorCylinderLight_H