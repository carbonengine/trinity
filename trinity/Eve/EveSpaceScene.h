#pragma once
#ifndef EveSpaceScene_H
#define EveSpaceScene_H


#include "include/ITr2Scene.h"
#include "include/IEveBallpark.h"
#include "TriFrustum.h"
#include "SpaceObject/EveSpaceObject2.h"
#include "TriRenderBatch.h"
#include "Tr2PickBuffer.h"
#include "Curves/TriCurveSet.h"
#include "include/ITriFunction.h"
#include "EvePlanet.h"
#include "EveStarfield.h"
#include "include/ITr2MultiPassScene.h"
#include "Tr2Variable.h"
#include "EveUpdateContext.h"
#include "Tr2QuadRenderer.h"
#include "Tr2LightManager.h"
#include "PostProcess/Tr2PostProcess2.h"
#include "Include/ITr2NamedPredicate.h"
#include "Eve/EveComponentRegistry.h"
#include "Tr2Variable.h"
#include "TriFrustumOrtho.h"
#include "Tr2ShadowMap.h"
#include "Eve/SpaceObject/Children/EveChildCloud2.h"

class TriProjection;
class TriView;

// Objects are allowed to unload their resources if they're out of view for
// some time. This can help reduce memory use.
extern bool g_eveIsSpaceObjectResourceUnloadingEnabled;

// Time threshold for resource unloading (in seconds).
extern float g_eveSpaceObjectResourceUnloadingTimeThreshold;

// Object itself only renders if estimated pixel diameter is above
// this threshold. Note that attachments may still render, in particular
// turret firing effects, or light glows as they may be more noticeable from afar.
extern float g_eveSpaceSceneVisibilityThreshold;

// Object itself renders with low detail geometry (if available) if estimated pixel
// diameter is above this threshold. Note that attachments may still render, in particular
// turret firing effects, or light glows as they may be more noticeable from afar.
extern float g_eveSpaceSceneLowDetailThreshold;
extern float g_eveSpaceSceneMediumDetailThreshold;
extern float g_eveSpaceSceneHighDetailThreshold;

// Setting for what reflection mode is used
extern int g_eveReflectionMode;

// raytracing shadow setting
extern bool g_eveSpaceSceneRaytracedShadows;

struct ShadowReceiver;

BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE_INTERFACE( ITr2TextureProvider );
BLUE_DECLARE( EveLensflare );
BLUE_DECLARE_VECTOR( EveLensflare );
BLUE_DECLARE( Tr2ShadowMap );
BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2SSAO );
BLUE_DECLARE( EveTransform );
BLUE_DECLARE_VECTOR( EveTransform );
BLUE_DECLARE( EveDistanceField );
BLUE_DECLARE_VECTOR( EveDistanceField );
BLUE_DECLARE( EveSceneStaticParticles );
BLUE_DECLARE_VECTOR( EveSceneStaticParticles );
BLUE_DECLARE( Tr2ShaderBuffer );
BLUE_DECLARE( Tr2DataTextureManager );
BLUE_DECLARE( Tr2GpuParticleSystem );
BLUE_DECLARE( Tr2ExternalParameter );
BLUE_DECLARE_VECTOR( Tr2ExternalParameter );
BLUE_DECLARE( Tr2ImpostorManager );
BLUE_DECLARE( Tr2DebugRenderer );
BLUE_DECLARE( EveEffectRoot2 );
BLUE_DECLARE( Tr2ReflectionProbe );
BLUE_DECLARE( EvePlanet );
BLUE_DECLARE_VECTOR( EvePlanet );
BLUE_DECLARE( EveVirtualCameraSystem );
BLUE_DECLARE( Tr2GpuBuffer );
BLUE_DECLARE( Tr2GpuStructuredBuffer );
BLUE_DECLARE( Tr2TextureReference );
BLUE_DECLARE( Tr2VolumetricsRenderer );
BLUE_DECLARE( Tr2RaytracingManager );

BLUE_CLASS( EveSpaceScene ) :
	public ITr2Scene,
	public ITr2MultiPassScene,
	public IInitialize,
	public INotify,
	public Tr2DeviceResource,
	public IListNotify,
	public ITr2NamedPredicate
{
public:
	EXPOSE_TO_BLUE();

	EveSpaceScene( IRoot* lockobj = NULL );
	~EveSpaceScene();

	static bool IsMeshUnloadingEnabled();

	IRoot* PickObject( int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, Be::OptionalWithDefaultValue<Tr2PickTypes, PICK_TYPE_PICKING | PICK_TYPE_OPAQUE> filter ); // for use by python, uses default immediate context
	IRoot* PickObjectAndArea( int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, unsigned int& areaID, Tr2PickTypes pickTypes, Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2Scene
	virtual void Update( Be::Time realTime, Be::Time simTime );
	virtual void Render( Tr2RenderContext& renderContext );
	virtual void RenderDebugInfo( Tr2RenderContext & renderContext );

	RenderPassResult RenderPass( PassType pass, Tr2RenderContext & renderContext );
	void RenderMainPass( Tr2RenderContext & renderContext, Tr2RenderContextEnum::CullMode cullmode = Tr2RenderContextEnum::CULLMODE_CW );
	void RenderDepthPass( Tr2RenderContext & renderContext );
	void RenderBackgroundPass( Tr2RenderContext & renderContext );
	void RenderReflectionPass( Tr2RenderContext & renderContext );
	void BeginRender( Tr2RenderContext & renderContext );
	void EndRender( Tr2RenderContext & renderContext );
	void Render3DUI( Tr2RenderContext & renderContext );
	void PopulateAndApplyPerFrameData( Tr2RenderContext & renderContext );
	void ApplyUpscalingToPerFrameData( uint32_t width, uint32_t height, Tr2RenderContext & renderContext );

	void GatherBatches( Tr2RenderContext & renderContext );
	void PrepareRaytracedShadows( Tr2RenderContext & renderContext );

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * val );

	//////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified(
		long event,		// BLUELISTEVENT values
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const struct IList* theList );

	//////////////////////////////////////////////////////////////////////////
	// ITr2NamedPredicate
	bool GetPredicate( const char* name ) const override;

	// all eve-specific visualize methods
	enum EveVisualizeMethod
	{
		VM_NONE,

		// vertex values
		VM_TEXCOORD0,
		VM_TEXCOORD1,

		// general
		VM_WHITE,
		VM_OVERDRAW,
		VW_WIREFRAME,

		VW_LIGHT_COUNT,

		VM_COUNT
	};

	struct PerThreadAccumulator : TriRenderBatchAccumulator<EffectKeyGenerator>
	{
		PerThreadAccumulator() :
			TriRenderBatchAccumulator<EffectKeyGenerator>( Tr2Renderer::GetPoolAllocator() )
		{
		}
	};

	// BatchMaps are used to store batch accumulators for rendering
	typedef std::map<TriBatchType, ITriRenderBatchAccumulator*> BatchMap;
	typedef std::map<TriBatchType, Tr2EnumerableThreadSpecific<PerThreadAccumulator>> PerThreadBatchMap;

	Tr2ShLightingManagerPtr GetShLightingManager() const;
	void SetShLightingManager( Tr2ShLightingManager * manager );

	IEveSpaceObject2Vector& Objects()
	{
		return m_objects;
	}
	Tr2PostProcess2Ptr GetPostProcess();
	Tr2ShaderBufferPtr GetPostProcessPSBuffer();
	void SetVelocityMap( Tr2RenderTargetPtr velocityMap );
	Tr2DepthStencil* GetDepth();
	Matrix GetReprojectionMatrix() const;


	enum ShadowQuality
	{
		SHADOW_DISABLED,
		SHADOW_LOW,
		SHADOW_HIGH,
		SHADOW_RAYTRACED
	};

protected:
	// Data shared between the different rendering method chunks
	struct FrameData
	{
		Matrix projection;

		std::vector<ShadowReceiver> objectsReceivingShadow;
	};
	FrameData m_frameData;

	// Per-frame pixel constants for rendering scene
	struct SunData
	{
		Vector3 DirWorld;
		float unused_pad0;
		Color DiffuseColor;
	};

	struct ShadowInfo
	{
		ShadowInfo( float radius, IEveShadowCaster* caster, Tr2PerObjectData* perObjectData ):
			radius( radius ),
			caster( caster ),
			perObjectData( perObjectData )
		{
		}

		float radius;
		IEveShadowCaster* caster;
		Tr2PerObjectData* perObjectData;
	};

	// Per-frame pixel constants for rendering scene
	struct PerFramePSData
	{
		Matrix ViewInverseTransposeMat;
		Matrix ViewMat;
		Matrix EnvMapRotationMat;

		SunData Sun;
		Vector3 AmbientColor;
		float ReflectionIntensity;
		Vector4 FogColor;

		// ViewportOffsetSize
		Vector2 ViewportOffset;
		Vector2 ViewportSize;

		// RenderTargetData
		Vector2 TargetResolution;
		float DepthMapSampleCount;
		float Debug;

		// ShadowMapSettings
		Vector4 ShadowMapSettings;

		// ShadowMapSettings2
		Vector2 ShadowCameraRange;
		float ShadowLightness;
		float unused1;

		// ProjectionData
		Vector2 ProjectionToView;
		Vector2 FovXY;

		// MiscData
		float Time;
		float SceneMipLodBias;
		float Upscaling;
		float GammaBrightness;

		uint32_t FrameIndex;
		uint32_t Jittering; //0 if off, 1 if on
		uint32_t unused3;
		uint32_t unused4;

		float VolumetricSlices[4];

		// Cascaded shadow maps
		Vector4 ShadowMapValues[4]; // x = zFar value[0], y = zFar value[1], z = zFar value[2], w = zFar value[3]..etc
		Matrix ShadowMatrixVal[SHADOW_FRUSTUM_COUNT];
		Vector4 SplitInfo;
		Matrix ProjectionInverseMat;
	};

	Matrix m_viewLast, m_projectionLast;
	Matrix m_jitterMatrix;

	// Per-frame vertex constants for rendering scene
	struct PerFrameVSData
	{
		Matrix ViewInverseTransposeMat;
		Matrix ViewProjectionMat;
		Matrix ViewMat;
		Matrix ProjectionMat;
		Matrix ShadowViewMat;
		Matrix ShadowViewProjectionMat;
		Matrix EnvMapRotationMat;
		Matrix ViewProjectionLast;
		Matrix ViewLast;
		Matrix ProjLast;

		// pass sun data to vertexshader, so certain lighting-calculations can be done per-vertex and not per-pixel
		SunData Sun;
		Vector3 FogFactors;
		float   pad;
		// pass resolution to vertexshader, can be usefull in some crazy shaders
		Vector2 TargetResolution;
		// pass fov x and y
		Vector2 FovXY;
		// used to reconstruct clip positions without viewport projection adjustment
		Vector4 ViewportAdjustment;

		float Time;
		float Upscaling;
		Vector2 ViewportSize;
	};

	struct EvePlanetPerObjData
	{
		// 2 largest planets in scene for shadow casting
		Vector4 planetSphere[2];
	};

	struct PlanetInfo
	{
		Vector4 sphere = Vector4(0,0,0,0);
		float pixelSize = 0.0;
	};
	EvePlanetPerObjData m_planetPerObjData;
	Tr2ShaderBufferPtr m_planetPerObjBuffer;

	struct PostProcessPSData
	{
		Vector3 OriginShift;
		float DeltaT;
	};


	PostProcessPSData m_postProcessPSData;
	Tr2ShaderBufferPtr m_postProcessPSBuffer;

	void UpdatePostProcessPSData();

	void PopulatePerFrameVSData( PerFrameVSData & data, Tr2RenderContext & renderContext );
	void PopulatePerFramePSData( PerFramePSData & data, Tr2RenderContext & renderContext );
	void ApplyPerFrameData( Tr2RenderContext & renderContext );

	void UpdatePlanets( const EveUpdateContext & updateContext );
	void RenderPlanets( Tr2RenderContext & renderContext );

	void RenderDistortion( Tr2RenderContext & renderContext );

	Matrix CreatePlanetViewMatrix( const Matrix& original );
	void SetupPlanetsAsShadowCaster( Tr2RenderContext & renderContext );

	void GetPickingResults( Tr2PickBuffer & pickBuffer, Tr2RenderContext & renderContext, unsigned short& objId, unsigned short& areaId, float& depth );
	void DecodeBufferPixel( const void* pBuffer, unsigned short& objId, unsigned short& areaId, float& depth ) const;

	// Batch gathering and preparation
	void GetAllBatchesFromRenderables( std::vector<ITr2Renderable*> & objectRenderables, Tr2RenderableSortList & objectsWithTransparencies, BatchMap & batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	void GetOpaqueBatchesFromRenderables( std::vector<ITr2Renderable*> & objectRenderables, BatchMap & batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	void GetDepthBatchesFromRenderables( std::vector<ITr2Renderable*> & objectRenderables, BatchMap & batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	void GetTransparentBatchesFromRenderables( std::vector<ITr2Renderable*> & objectRenderables, Tr2RenderableSortList & objectsWithTransparencies, BatchMap & batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	void PrepareTransparentBatch( Tr2RenderableSortList & objectsWithTransparencies, BatchMap & batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );

	// Batch rendering
	void RenderOpaqueBatches( BatchMap & batches, Tr2RenderContext & renderContext );
	void RenderTransparentBatches( BatchMap & batches, Tr2RenderContext & renderContext );
	void RenderTransparentBatches2( BatchMap & batches, Tr2RenderContext & renderContext, bool pass );
	bool RenderDistortionBatches( BatchMap& batches, Tr2RenderContext& renderContext );

	// Utility rendering functions
	void RenderBatch( ITriRenderBatchAccumulator * batch,
					  Tr2EffectStateManager::RenderingMode rm,
					  Tr2RenderContext & renderContext );

	void RenderRenderables( const std::vector<ITr2Renderable*>& renderables,
							ITriRenderBatchAccumulator* batch,
							TriBatchType batchType,
							Tr2EffectStateManager::RenderingMode rm,
							Tr2RenderContext& renderContext,
							Tr2RenderReason reason = TR2RENDERREASON_NORMAL );

	void UpdateSceneFromScript( Be::Time time );
	void ReregisterEntities();
	void ClearComponentRegistry();

	void RenderVolumetrics( Tr2RenderContext & renderContext );

protected:
	bool m_display;
	bool m_update;
	bool m_displayShadowMap;
	bool m_backgroundRenderingEnabled;
	TriFrustumOrtho m_shadowFrustums[SHADOW_FRUSTUM_COUNT];
	TriFrustum m_cameraFrustums[SHADOW_FRUSTUM_COUNT];

	float m_planetScale;
	float m_planetCameraScale;

	//cascaded shadow map
	Tr2ShadowMapPtr m_cascadedShadowMap;
	TriTextureResPtr m_emptyShadowMap;

	PIEveSpaceObject2Vector	m_backgroundObjects;
	PEvePlanetVector		m_planets;
	PIEveSpaceObject2Vector m_objects;
	PIEveSpaceObject2Vector m_uiObjects;
	IEveSpaceObject2Ptr		m_warpTunnel;
	PTriCurveSetVector		m_curveSets;
	PEveLensflareVector		m_lensflares;
	EveUpdateContext		m_updateContext;
	PEveDistanceFieldVector m_distanceFields;

	// Primary batches, gathered in BeginRender and
	// cleared in EndRender
	BatchMap m_primaryBatches;
	// Secondary batches used to render planets, stars, shadowed objects
	// and so forth. Should be finalized and cleared each time they're used.
	BatchMap m_secondaryBatches;
	PerThreadBatchMap m_perThreadBatches;

	// Utility batches.
	std::vector<TriPoolAllocator> m_shadowAllocators;
	std::vector<std::unique_ptr<ITriRenderBatchAccumulator>> m_shadowBatches;
	ITriRenderBatchAccumulator* m_pickingBatches;

	Tr2EffectPtr m_backgroundEffect;

	Quaternion m_envMapRotation;

	std::string m_envMapResPath;
	TriVariable* m_envMapHandle;
	ITr2TextureProviderPtr m_envMapTextureRes;

	std::string m_envMap1ResPath;
	ITr2TextureProviderPtr m_envMap1;

	std::string m_envMap2ResPath;
	ITr2TextureProviderPtr m_envMap2;

	std::string m_envMap3ResPath;
	ITr2TextureProviderPtr m_envMap3;

	std::string m_lowQualityNebulaResPath;
	ITr2TextureProviderPtr m_lowQualityNebula;

	std::string m_lowQualityNebulaMixResPath;
	ITr2TextureProviderPtr m_lowQualityNebulaMix;

	Tr2Variable m_envMap1Var;
	Tr2Variable m_envMap2Var;

	Tr2Variable m_reflectionMapVar;
	Tr2Variable m_reflectionMaskMapVar;

	Tr2Variable m_envMapTransformVar;
	Tr2Variable m_reflectionMapTransformVar;
	Tr2Variable m_suncVecVar;
		
	Tr2RenderTargetPtr m_colorMap;
	Tr2RenderTargetPtr m_opaqueColorMap;
	Tr2DepthStencilPtr m_depthMap;
	Tr2Variable m_depthMapVar;


	TriVariable* m_ssaoMapHandle;
	Tr2RenderTargetPtr m_normalMap;
	Tr2SSAOPtr m_ssao;

	Tr2RenderTargetPtr m_distortionMap;
	Tr2RenderTargetPtr m_velocityMap;

	// has the velocity map been written to?
	bool m_velocityMapDirty; 

	void ClearRenderTargetIfNoBatches( Tr2RenderTarget* rt, uint32_t slot, Tr2RenderContext& renderContext, size_t batchCount );

	SunData m_sunData;
	Color m_sunColor;
	Color m_sunColorWithDynamicLights;
	bool m_useSunColorWithDynamicLights;

	Color m_ambientColor;
	Color m_fogColor;
	float m_nebulaIntensity;
	Tr2Variable m_nebulaIntensityVar;

	float m_fogStart; // Depth at which fogging starts
	float m_fogEnd; // Depth at which fog does not get stronger
	float m_fogMax; // Maximum strength of fog, range [0,1], at m_fogEnd distance.

	PerFramePSData m_perFramePS;
	PerFrameVSData m_perFrameVS;

	EveVisualizeMethod m_visualizeMethod;
	float m_perFrameDebug;

	PEveSceneStaticParticlesVector m_staticParticles;

	Tr2DataTextureManagerPtr m_dataTextureMgr;

	// For tracking the sunlight direction
	ITriVectorFunctionPtr m_sunBall;

	// ballpark of this scene
	IEveBallparkPtr m_ballpark;

	PTr2ExternalParameterVector m_externalParameters;

	Tr2DebugRendererPtr m_debugRenderer;

	Vector3 PickInfinity( int x, int y, Matrix proj, Matrix view );

	//For GPU particles
private:
	Tr2GpuParticleSystem* GetGpuParticleSystem() const
	{
		return m_updateContext.GetGpuParticleSystem();
	}
	void SetGpuParticleSystem( Tr2GpuParticleSystem * ps )
	{
		m_updateContext.SetGpuParticleSystem( ps );
	}

	bool HasReflectionProbe() const;

	Be::Time m_updateTime;
	EveSpaceObject2Ptr m_egoBall;

	Tr2ConstantBufferAL	m_perFrameVSBuffer;
	Tr2ConstantBufferAL	m_perFramePSBuffer;
	Tr2ConstantBufferAL	m_shadowPerFrameVSBuffer;

	// Cascaded shadows
	void SetupCascadedShadows( Tr2RenderContext & renderContext );
	void DisableShadows();

	// Picking
	void SetupTransformsForPicking( float fx, float fy, TriProjection* proj,  TriView* view, TriViewport* viewport, Tr2RenderContext& renderContext );
	void GetPickingObjectsToRender( std::vector<ITr2Renderable*> & pickableRenderObjects );

	Tr2PickBuffer m_pickBuffer;
	EveStarfieldPtr m_starfield;

	struct VisualizerEffect
	{
		enum VisualizerType
		{
			PIXEL_SHADER_REPLACEMENT,
			FULL_SCREEN_QUAD,
			FULL_SCREEN_QUAD_OVERLAY,
		};

		VisualizerEffect() :
			type( PIXEL_SHADER_REPLACEMENT )
		{
		}

		Tr2EffectPtr effect;
		VisualizerType type;
	};

	VisualizerEffect m_visualizerEffects[VM_COUNT];

	void UpdateVariableStore();
	void ClearVariableStore();

	void ClearBatches( BatchMap & batches );
	void FinalizeBatches( BatchMap & batches );

	virtual void ReleaseResources( TriStorage s );
	virtual bool OnPrepareResources();

	void UpdateShLighting(
		const std::vector<IEveSpaceObject2*>& objectsNotReceivingShadow );

	void UpdateQuadRenderer(
		const TriFrustum& frustum,
		const std::vector<IEveSpaceObject2*>& objectsNotReceivingShadow,
		Tr2RenderContext& renderContext );

	void UpdateQuadRenderer(
		const TriFrustum& frustum,
		PIEveSpaceObject2Vector& objects,
		Tr2RenderContext& renderContext );

	Tr2QuadRenderer* GetQuadRenderer() const;

	void UpdateImpostors( Tr2RenderContext & renderContext );

	enum BackgroundRenderingReason
	{
		BACKGROUND_RENDER_COLOR,
		BACKGROUND_RENDER_REFLECTION,
	};

	void RenderBackgroundPassObjects( Tr2RenderContext & renderContext, BackgroundRenderingReason reason );

	Tr2ShLightingManagerPtr m_shLightingManager;

	float m_upscalingAmount;
	bool m_usingUpscaling; 
	Vector4 m_jitter; // xy: projection offset, zw: pixel offset

	bool m_hasBackgroundDistortionBatches;
	bool m_hasForegroundDistortionBatches;

	void Jitter( Tr2RenderContext& renderContext );

	Tr2ImpostorManagerPtr m_impostorManager;

	EveEffectRoot2Ptr m_cameraAttachmentParent;

	IRoot* GetCameraAttachments() const;

	Tr2PostProcess2Ptr m_postProcess;
	Tr2ReflectionProbePtr m_reflectionProbe;
	EveVirtualCameraSystemPtr m_virtualCameraSystem;

	float m_reflectionIntensity;
	float m_reflectionBackLightingContrast;
	Color m_reflectionBackLightingColor;

	BlueSharedString m_name;

	// Object to gather all components
	EveComponentRegistryPtr m_componentRegistry;

	Tr2VolumetricsRendererPtr m_volumetricsRenderer;

	bool m_dynamicObjectReflectionEnabled;

	ShadowMap::SplitSetup m_splitSetup[SHADOW_FRUSTUM_COUNT];
	// Cascaded shadow debugging
	bool m_freezeFrustum;
	Matrix m_shadowView;
	// for "fake" scenes like ship fitting etc
	bool m_enableShadows;

	ShadowQuality m_shadowQuality;

	const BlueSharedString m_shadowCascadedName = BlueSharedString( "SHADOW_CASCADED" );
	const BlueSharedString m_shadowRaytracedName = BlueSharedString( "SHADOW_RT" );
	BlueSharedString m_shadowCascadedValue;
	BlueSharedString m_shadowRaytracedValue;
	bool m_usingRaytracedShadows;

	void RenderVolumetricShadowMap( Tr2RenderContext & renderContext );
	void RenderIntoCloudShadowMap( Tr2RenderContext & renderContext, const ITr2VolumetricRenderable::ShadowInfo* cloudShadowInformation, std::vector<IEveShadowCaster*> shadowCasters );

	// raytracing
	Tr2RaytracingManagerPtr m_rtManager;
	bool m_enableRaytracing;

	// projection matrices
	Matrix m_projection;
	Matrix m_jitteredProjection;

	// reprojection matrix
	Matrix m_reprojectionMatrix;
};

TYPEDEF_BLUECLASS( EveSpaceScene );

// Description:
//	Helper function to get batches from renderables, can handle the variation between GetOpaque,
//	GetAll, GetTransparent.  Also usable by other scenes that want to get renderable batches.
// Arguments:
//	objectRenderables, renderableCount - pointer and count to list of renderables to query
//	objectsWithTransparencies - if not null, make a separate list of objects with transparent
//								batches, with sort value set up
//	batches - map holding the outcome
//	allocator - allocator to use
//	batchTypes, batchTypeCount - which batches to query
//  reason - Why do we need the renderabes (either normal or reflections)
void GetBatchesFromRenderables( ITr2Renderable** objectRenderables, const unsigned renderableCount, Tr2RenderableSortList* objectsWithTransparencies, EveSpaceScene::BatchMap& batches, const TriBatchType* batchTypes, const unsigned batchTypeCount, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );


#endif // EveSpaceScene_H