// Copyright © 2023 CCP ehf.

#pragma once

#include "ITr2Renderable.h"
#include "Eve/IEveSpaceObject2.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Tr2DebugRenderer.h"
#include "Tr2DepthStencil.h"
#include "TriFrustumOrtho.h"
#include "Utilities/BoundingBox.h"
#include "../../EveEntity.h"
#include "../../../ITr2VolumetricRenderable.h"
#include "Lights/ITr2LightOwner.h"

BLUE_DECLARE_INTERFACE( ITriVectorFunction );
BLUE_DECLARE_INTERFACE( ITriQuaternionFunction );
BLUE_DECLARE( Tr2Light );
BLUE_DECLARE_VECTOR( Tr2Light );
BLUE_DECLARE( Tr2TextureReference );
BLUE_DECLARE( Tr2TextureAnimation );


// --------------------------------------------------------------------------------
// Description:
//   A space object used for rendering volumetric clouds. Renders a camera-aligned
//   plane at the back of the object's bounding sphere using provided effect.
// --------------------------------------------------------------------------------
BLUE_CLASS( EveChildCloud2 ) :
	public ITr2VolumetricRenderable,
	public Tr2DeviceResource,
	public IInitialize,
	public INotify,
	public IListNotify,
	public IEveSpaceObjectChild,
	public ITr2DebugRenderable,
	public ITr2Renderable,
	public ITr2LightOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveChildCloud2( IRoot* lockobj = NULL );
	~EveChildCloud2();

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var * value );

	//////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	virtual const char* GetName() const override;
	virtual void SetName( const char* name ) override;
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	virtual void GetLocalToWorldTransform( Matrix & transform ) const override;
	virtual void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) override;
	virtual void GetRenderables( std::vector<ITr2Renderable*> & renderables ) override;
	virtual bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const override;
	virtual void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) override
	{
	}
	virtual void ChangeLOD( Tr2Lod lod ) override
	{
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2LightOwner
	void GetLights( Tr2LightManager & lightManager ) const override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2VolumetricRenderable
	float GetSortValue( const TriFrustum& frustum ) override;
	void GetVolumetricBatches( const TriFrustum& frustum, ITriRenderBatchAccumulator* batches ) override;
	bool UpdateVolumetricLightmap( Tr2RenderContext & renderContext ) override;
	void SetSceneInformation( const SceneInformation& sceneInformation ) override;
	void GetVolumetricShadowBatches( ITriRenderBatchAccumulator * batches ) override;
	void GetVolumetricShadowInfo( ShadowInfo & shadowInfo, Vector3 sunDir ) override;
	bool PrepareCloudShadowMap( Tr2RenderContext & renderContext ) override;
	void SetCloudShadowMapHandle() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	bool IsVisible( const EveUpdateContext& updateContext ) const override;
	void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ) override;
	bool HasTransparentBatches() override;
	float GetSortValue() override;
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions & options );
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer );

	void ClearVariableStore();

	void RegisterComponents() override;

	void SetupShadowFrustum( ShadowInfo & shadowInfo, Vector3 sunDir );

	bool IsLightmapDirty() const;
	void MarkLightmapDirty( bool );

	struct LightData
	{
		Vector3 position;
		float radius;
		Vector3 color;
		float innerRadius;
	};

	struct PerObjectData
	{
		Matrix world;
		Matrix projectionInv;
		Matrix worldViewInv;
		uint32_t lightmapDimensions[4];
		uint32_t noiseConfig[4];
		Vector3 sunDirection;
		float depthSlice0;
		Vector3 viewPosition;
		float depthSlice1;
		Vector3 viewDirection;
		float depthSlice2;
		Vector3 relativeScaling;
		float lodFactor;
		Vector2 targetInvSize;
		Vector2 unused2;
		LightData lights[4];
		Vector4 mapOffsets[3];
		Matrix lightViewProj;
	};

private:
	bool OnPrepareResources();
	void PopulatePerObjectData( PerObjectData & data, float screenSize ) const;
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator, float screenSize );
	void CreateEmptyLightMap();
	bool HasValidTransform() const;

	// data for positioning
	Matrix m_localTransform;
	Matrix m_worldTransform;
	Vector3 m_prevSunDirection;
	Vector3 m_localSunDirection;
	float m_depthSlices[SceneInformation::depthSliceCount];

	// bounding sphere
	CcpMath::Sphere m_boundingSphere;

	Tr2EffectPtr m_effect;
	Tr2EffectPtr m_reflectionEffect;
	Tr2VariableStorePtr m_variableStore;
	unsigned m_effectHash;

	uint32_t m_noiseTextureSize;

	Vector3 m_scaling;
	Vector3 m_translation;
	Quaternion m_rotation;

	uint32_t m_declaration;
	Tr2BufferAL m_vertexBuffer;
	Tr2BufferAL m_indexBuffer;

	PTr2LightVector m_lights;

	Tr2TextureReferencePtr m_emptyLightMap;
	Tr2TextureReferencePtr m_lightMap;
	Tr2TextureAnimationPtr m_animation;

	EntityComponents::ReflectionMode m_reflectionMode;
	Tr2VolumerticQuality m_minVisibleQuality;
	Tr2VolumerticQuality m_currentQuality;

	Tr2DepthStencilPtr m_shadowMapDS;
	TriVariable* m_depthShadowMapHandle;

	std::string m_name;
	bool m_display;
	bool m_castShadows;
	bool m_receiveShadows;
	bool m_lightmapDirty;
	bool m_renderedLastFrame;
	// Has UpdateSyncronous/UpdateAsyncronous been called: until it has, the object cannot be rendered
	bool m_hasUpdated = false;

	float m_sortingModifier;
	float m_minScreenSize;
	float m_adjustedMinScreenSize;

	uint32_t m_lightmapWidth;
	uint32_t m_lightmapHeight;
	uint32_t m_lightmapDepth;
	uint32_t m_lightmapDirtyOffset;
	float m_lightmapSizeScale;

	uint32_t m_targetWidth;
	uint32_t m_targetHeight;

	Vector3 m_mapTiling[3];
	Vector3 m_mapOffsets[3];

	const Vector3 m_unitCube[8] = {
		//The unit cube in DirectX is from( -1, -1, 0 ) to ( 1, 1, 1 )
		Vector3( -1, -1, 0 ), // vertex 0
		Vector3( -1, 1, 0 ), // vertex 1
		Vector3( 1, 1, 0 ), // etc..
		Vector3( 1, -1, 0 ),
		Vector3( -1, -1, 1 ),
		Vector3( -1, 1, 1 ),
		Vector3( 1, 1, 1 ),
		Vector3( 1, -1, 1 )
	};

	Matrix m_lightViewProj;
	float m_zFar;
	unsigned int m_shadowMapSize;
};

TYPEDEF_BLUECLASS( EveChildCloud2 );
BLUE_DECLARE_VECTOR( EveChildCloud2 );
