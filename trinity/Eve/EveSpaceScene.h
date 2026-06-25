// Copyright © 2023 CCP ehf.

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
#include "EvePlanet.h"
#include "EveStarfield.h"
#include "include/ITr2MultiPassScene.h"
#include "Tr2Variable.h"
#include "EveUpdateContext.h"
#include "Tr2QuadRenderer.h"
#include "Tr2LightManager.h"
#include "PostProcess/Tr2PostProcess2.h"
#include "Eve/EveComponentRegistry.h"
#include "TriFrustumOrtho.h"
#include "Tr2ShadowMap.h"
#include "Eve/SpaceObject/Children/EveChildCloud2.h"
#include "PostProcess/ITr2PostProcessOwner.h"
#include "../Tr2VolumetricsRenderer.h"
#include "../Tr2RuntimeGpuBuffer.h"
#include "EvePicking.h"

#include <ITriFunction.h>

class TriProjection;
class TriView;
class Tr2GpuResourcePool;

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

struct ShadowReceiver;

BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE_INTERFACE( ITr2TextureProvider );
BLUE_DECLARE( EveLensflare );
BLUE_DECLARE_VECTOR( EveLensflare );
BLUE_DECLARE( Tr2ShadowMap );
BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2SSAO );
BLUE_DECLARE( Tr2SSSSS );
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
BLUE_DECLARE( Tr2PostProcessAttributes );
BLUE_DECLARE_VECTOR( Tr2PostProcessAttributes );

class EveInstancedMeshManager;



BLUE_CLASS( EveSpaceScene ) :
	public ITr2Scene,
	public IInitialize,
	public INotify,
	public Tr2DeviceResource,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	EveSpaceScene( IRoot* lockobj = NULL );
	~EveSpaceScene();

	static bool IsMeshUnloadingEnabled();

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2Scene
	virtual void Update( Be::Time realTime, Be::Time simTime );
	virtual void Render( Tr2RenderContext & renderContext );
	virtual void RenderDebugInfo( Tr2RenderContext & renderContext );


	struct ShadowResources
	{
		// Screen space shadow map (greyscale): valid for both shadow maps and raytraced shadows
		Tr2GpuResourcePool::Texture shadowMap;
		// Cascaded shadow map depth buffer atlas: only valid for shadow maps
		Tr2GpuResourcePool::Texture cascadedShadowDepth;
		// Screen space shadow indices for point lights : only valid for raytraced shadows
		Tr2GpuResourcePool::Texture pointLightShadowMap;
		// Depth buffer atlas for point light shadows : only valid for shadow maps
		Tr2GpuResourcePool::Texture pointLightShadowDepth;
	};

	ShadowResources RenderShadows( const Tr2TextureAL& depthMap, const Tr2TextureAL& normalMap, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext );
	bool RenderMainPass(
		const Tr2TextureAL& colorMap,
		const Tr2TextureAL& depthMap,
		const Tr2TextureAL& distortionMap,
		const Tr2TextureAL& velocityMap,
		const Tr2TextureAL& opaqueColorMap,
		Tr2GpuResourcePool& gpuResourcePool,
		Tr2RenderContext& renderContext );
	void RunLensflareOcclusionQueries( const Tr2TextureAL& depthMap, Tr2RenderContext& renderContext );
	void RenderDepthPass( const Tr2TextureAL& depthMap, const Tr2TextureAL& normalMap, const Tr2TextureAL& customStencil, Tr2RenderContext& renderContext, const BlueSharedString& techniqueName = BlueSharedString( "Depth" ) );

	bool RenderBackgroundPass( const Tr2TextureAL& depthMap, const Tr2TextureAL& distortionMap, const Tr2TextureAL& velocityMap, Tr2RenderContext& renderContext );
	void RenderReflectionPass( Tr2GpuResourcePool & gpuResourcePool, Tr2RenderContext & renderContext );
	void BeginRender( bool enableDistortion, Tr2RenderContext& renderContext );
	void EndRender( Tr2RenderContext & renderContext );
	void Render3DUI( Tr2RenderContext & renderContext );
	void PopulateAndApplyPerFrameData( Tr2RenderContext & renderContext );
	void ApplyUpscalingToPerFrameData( uint32_t width, uint32_t height, Tr2RenderContext& renderContext );

	void GatherBatches( bool includeDistortions, Tr2RenderContext& renderContext );
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
		long event, // BLUELISTEVENT values
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const struct IList* theList );

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

	Matrix GetReprojectionMatrix() const;

	// Per-frame pixel constants for rendering scene
	struct SunData
	{
		Vector3 DirWorld;
		float unused_pad0;
		Color DiffuseColor;
	};

	struct ShadowInfo
	{
		ShadowInfo( float radius, IEveShadowCaster* caster, Tr2PerObjectData* perObjectData ) :
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
		uint32_t ShadowQuality;

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
		float InverseShadowMapAtlasSize; // shadow map atlas for dynamic lights
		uint32_t ShadowMapAtlasEntryMinSizeLog2; // shadow map atlas for dynamic lights

		float VolumetricSlices[4];

		// Cascaded shadow maps
		Vector4 ShadowMapValues[4]; // x = zFar value[0], y = zFar value[1], z = zFar value[2], w = zFar value[3]..etc
		Matrix ShadowMatrixVal[SHADOW_FRUSTUM_COUNT];
		Vector4 SplitInfo;
		Matrix ProjectionInverseMat;

		Vector4 CascadeRanges[16];

		Tr2VolumetricsRenderer::FroxelPerFrameData FroxelFogData;
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
		float pad;
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
		Vector4 sphere = Vector4( 0, 0, 0, 0 );
		float pixelSize = 0.0;
	};
	EvePlanetPerObjData m_planetPerObjData;
	Tr2ShaderBufferPtr m_planetPerObjBuffer;

	void PopulatePerFrameVSData( PerFrameVSData & data, Tr2RenderContext & renderContext );
	void PopulatePerFramePSData( PerFramePSData & data, Tr2RenderContext & renderContext );
	void PopulatePerFramePSData( PerFramePSData & data, Tr2ShadowMap * shadowMap, Tr2RenderContext & renderContext );

	void ApplyPerFrameData( Tr2RenderContext & renderContext );

	void UpdatePlanets( const EveUpdateContext& updateContext );
	void RenderPlanets( Tr2RenderContext & renderContext );

	void RenderDistortion( Tr2RenderContext & renderContext );

	Matrix CreatePlanetViewMatrix( const Matrix& original );
	void SetupPlanetsAsShadowCaster( CcpMath::Sphere * planets, size_t maxPlanets );
	void SetupPlanetsAsShadowCaster( Tr2RenderContext & renderContext );

	// Batch gathering and preparation
	void GetAllBatchesFromRenderables( std::vector<ITr2Renderable*> & objectRenderables, Tr2RenderableSortList & objectsWithTransparencies, bool includeDistortions, BatchMap& batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	void GetOpaqueBatchesFromRenderables( std::vector<ITr2Renderable*> & objectRenderables, BatchMap & batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	void GetDepthBatchesFromRenderables( std::vector<ITr2Renderable*> & objectRenderables, BatchMap & batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	void GetTransparentBatchesFromRenderables( std::vector<ITr2Renderable*> & objectRenderables, Tr2RenderableSortList & objectsWithTransparencies, bool includeDistortions, BatchMap& batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	void PrepareTransparentBatch( Tr2RenderableSortList & objectsWithTransparencies, BatchMap & batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );

	// Batch rendering
	void RenderOpaqueBatches( BatchMap & batches, Tr2RenderContext & renderContext );
	void RenderTransparentBatches( BatchMap & batches, Tr2RenderContext & renderContext );
	void RenderTransparentBatches2( BatchMap & batches, Tr2RenderContext & renderContext, bool pass );
	bool RenderDistortionBatches( BatchMap & batches, const Tr2TextureAL& distortionMap, const Tr2TextureAL& depthMap, Tr2RenderContext& renderContext );

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

	std::pair<Tr2GpuResourcePool::Texture, Tr2GpuResourcePool::Texture> RenderVolumetrics( const Tr2TextureAL& depthMap, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext );

	void EnableShadowsInReflections( bool enable );
	bool IsShadowsInReflectionsEnabled() const;

	void GetLightMatrices( const Tr2LightManager::PerLightData& lightData, Matrix& projection, Matrix& view );

	void ProcessOutdatedRTAnimations( Tr2RenderContext & renderContext );

protected:
	bool m_display;
	bool m_update;
	bool m_backgroundRenderingEnabled;

	float m_planetScale;
	float m_planetCameraScale;

	//cascaded shadow map
	Tr2ShadowMapPtr m_cascadedShadowMap;
	Tr2ShadowMapPtr m_reflectionShadowMap;

	PIEveSpaceObject2Vector m_backgroundObjects;
	PEvePlanetVector m_planets;
	PIEveSpaceObject2Vector m_objects;
	PIEveSpaceObject2Vector m_uiObjects;
	IEveSpaceObject2Ptr m_warpTunnel;
	PTriCurveSetVector m_curveSets;
	PEveLensflareVector m_lensflares;
	EveUpdateContext m_updateContext;
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

	Tr2EffectPtr m_backgroundEffect;

	Quaternion m_envMapRotation;

	std::string m_envMapResPath;
	TriVariable* m_staticEnvMapHandle;
	ITr2TextureProviderPtr m_staticEnvMapTextureRes;
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

	CTr2RuntimeGpuBuffer m_sharedIndexVertexBufferWrapper;
	Tr2Variable m_sharedIndexVertexBufferVar;

	CTr2RuntimeGpuBuffer m_bakedMorphTargetBufferWrapper;
	Tr2Variable m_bakedMorphTargetBufferVar;

	Tr2Variable m_depthMapVar;

	Tr2SSSSSPtr m_sssss;

	// has the velocity map been written to?
	bool m_velocityMapDirty;

	void ClearRenderTargetIfNoBatches( const Tr2TextureAL& rt, uint32_t slot, Tr2RenderContext& renderContext, size_t batchCount );

	SunData m_sunData;
	Color m_sunColor;
	Color m_sunColorWithDynamicLights;
	bool m_useSunColorWithDynamicLights;

	Color m_ambientColor;
	Color m_fogColor;
	float m_nebulaIntensity;
	float m_backgroundReflectionIntensity;
	float m_defaultDiffuseRoughness;
	Tr2Variable m_nebulaIntensityVar;

	Color m_currentSunColor;
	float m_currentNebulaIntensity;

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

	Tr2ConstantBufferAL m_perFrameVSBuffer;
	Tr2ConstantBufferAL m_perFramePSBuffer;
	Tr2ConstantBufferAL m_shadowPerFrameVSBuffer;

	// Cascaded shadows
	ShadowResources SetupCascadedShadows( Tr2RenderReason renderReason, Tr2ShadowMap & shadowMap, const TriFrustum& viewFrustum, const Tr2TextureAL& depthMap, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext );


	// Picking

	ITriRenderBatchAccumulator* m_pickingBatches;

	void SetupTransformsForPicking( float fx, float fy, TriProjection* proj, TriView* view, TriViewport* viewport, Tr2RenderContext& renderContext );
	void GetPickingObjectsToRender( std::vector<ITr2Renderable*> & pickableRenderObjects );



public:
	IRoot* PickObject( int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, Be::OptionalWithDefaultValue<Tr2PickTypes, PICK_TYPE_PICKING | PICK_TYPE_OPAQUE> filter ); // for use by python, uses default immediate context
	IRoot* PickObjectAndArea( int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, uint32_t& areaID, Tr2PickTypes pickTypes, Tr2PrimaryRenderContext& renderContext );

	IRoot* PickAsyncObject( EvePickingContext * listener, int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, Be::OptionalWithDefaultValue<Tr2PickTypes, PICK_TYPE_PICKING | PICK_TYPE_OPAQUE> filter ); // for use by python, uses default immediate context
	IRoot* PickAsyncObjectAndArea( EvePickingContext * listener, int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, uint32_t& areaID, Tr2PickTypes pickTypes, Tr2PrimaryRenderContext& renderContext );

	void PerformPicking( EvePickingContext * listener, bool immediate, int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, Tr2PickTypes pickTypes, Tr2PrimaryRenderContext& renderContext );



protected:
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

	bool RenderBackgroundPassObjects( const Tr2TextureAL& depthMap, const Tr2TextureAL& distortionMap, Tr2RenderContext& renderContext, BackgroundRenderingReason reason );

	Tr2ShLightingManagerPtr m_shLightingManager;

	float m_upscalingAmount;
	Vector4 m_jitter; // xy: projection offset, zw: pixel offset

	void Jitter( Tr2RenderContext & renderContext );

	Tr2ImpostorManagerPtr m_impostorManager;

	EveEffectRoot2Ptr m_cameraAttachmentParent;

	IRoot* GetCameraAttachments() const;

	void UpdatePostProcessAttributes();
	BluePy GetPostProcessDebug() const;
	Tr2PostProcess2Ptr m_combinedPostProcess;

public:
	Tr2PostProcess2Ptr m_sceneDefaultPostProcess;
	Tr2PostProcessAttributesPtr m_sceneDefaultPostProcessAttributes;
	Tr2PostProcessAttributesPtr m_combinedPostProcessAttributes;
	BluePy m_postProcessDebug;

	Tr2ReflectionProbePtr m_reflectionProbe;
	EveVirtualCameraSystemPtr m_virtualCameraSystem;

	float m_reflectionIntensity;
	float m_reflectionBackLightingContrast;
	Color m_reflectionBackLightingColor;
	float m_currentReflectionIntensity;

	BlueSharedString m_name;

	// Dynamic light shadow maps
	bool PrepareShadowMapForLights( Tr2RenderContext & renderContext, const Tr2TextureAL& shadowMap );
	void RenderShadowMapForSpotLight(
		Tr2RenderContext & renderContext,
		const std::vector<IEveShadowCaster*>& shadowCasters,
		uint32_t shadowMapScale,
		uint32_t shadowMapOffsetX,
		uint32_t shadowMapOffsetY,
		const Vector3& lightPosition,
		const Matrix& view,
		const Matrix& projection,
		const Tr2TextureAL& shadowMap );
	void RenderShadowMapForLight( Tr2RenderContext & renderContext, const std::vector<IEveShadowCaster*>& shadowCasters, const Tr2LightManager::PerLightData& lightData, const Tr2TextureAL& shadowMap );
	void FinishRenderingShadowMapForLights( Tr2RenderContext & renderContext );

	// Object to gather all components
	EveComponentRegistryPtr m_componentRegistry;

	Tr2VolumetricsRendererPtr m_volumetricsRenderer;

	bool m_dynamicObjectReflectionEnabled;

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

	// Frame index when Update was last called
	uint64_t m_lastUpdateFrame = std::numeric_limits<uint64_t>::max();

public:
	// raytracing
	Tr2RaytracingManagerPtr m_rtManager;

	// projection matrices
	Matrix m_projection;
	Matrix m_jitteredProjection;

	// reprojection matrix
	Matrix m_reprojectionMatrix;

	std::unique_ptr<EveInstancedMeshManager> m_instancedMeshManager;

	friend class EveSpaceSceneRenderDriver;
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


void RegisterWithVariableStore( const EveSpaceScene::ShadowResources& shadowResources, Tr2GpuResourcePool& gpuResourcePool );

#endif // EveSpaceScene_H