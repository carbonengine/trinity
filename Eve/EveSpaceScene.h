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

BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( EveLensflare );
BLUE_DECLARE_VECTOR( EveLensflare );
BLUE_DECLARE( TriShadowMap );
BLUE_DECLARE( EveDustfieldConstraint );
BLUE_DECLARE( Tr2RenderTarget );
struct ShadowReceiver;
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

enum TAASampling { TAA_NONE=0, TAA_RANDOM=1, TAA_2X=2, TAA_3X=3, TAA_4X=4 };

class EveSpaceScene :
	public ITr2Scene,
	public ITr2MultiPassScene,
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

	IRoot* PickObject( int x, int y, TriProjection* proj,  TriView* view, TriViewport* viewport, 
		Be::OptionalWithDefaultValue<Tr2PickTypes, PICK_TYPE_PICKING | PICK_TYPE_OPAQUE> filter );	// for use by python, uses default immediate context
	IRoot* PickObjectAndArea( int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, unsigned int& areaID, Tr2PickTypes pickTypes, Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2Scene
	virtual void Update( Be::Time realTime, Be::Time simTime );
	virtual void Render( Tr2RenderContext& renderContext );
	virtual void RenderDebugInfo( Tr2RenderContext& renderContext );

	RenderPassResult RenderPass( PassType pass, Tr2RenderContext& renderContext );
	void RenderMainPass( Tr2RenderContext& renderContext );
	void RenderDepthPass( Tr2RenderContext& renderContext );
	bool RenderBackgroundPass( Tr2RenderContext& renderContext );
	void BeginRender( Tr2RenderContext& renderContext );
	void EndRender( Tr2RenderContext& renderContext );
	void PopulateAndApplyPerFrameData( Tr2RenderContext& renderContext );

	void GatherBatches( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();
	
	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	//////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified(
		long event,		// BLUELISTEVENT values
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

	// BatchMaps are used to store batch accumulators for rendering
	typedef std::map<TriBatchType, ITriRenderBatchAccumulator*> BatchMap;

	Tr2ShLightingManagerPtr GetShLightingManager() const;
	void SetShLightingManager( Tr2ShLightingManager* manager );

protected:
	// Data shared between the different rendering method chunks
	struct FrameData
	{
		Matrix projection;
		TriFrustum frustum;

		std::vector<ShadowReceiver> objectsReceivingShadow;
	};
	FrameData m_frameData;


	// Per-frame vertex constants for rendering shadows
	struct ShadowPerFrameVSData
	{
		Matrix ViewProjectionMat;
		Matrix ViewMat;

		Vector2 CameraRange;
		float padding[2];
	};

	// Per-frame pixel constants for rendering scene
	struct SunData
	{
		Vector3 DirWorld;
		float unused_pad0;
		Color DiffuseColor;
	};

	// Per-frame pixel constants for rendering scene
	struct PerFramePSData
	{
		Matrix ViewInverseTransposeMat;
		Matrix ViewMat;
		Matrix EnvMapRotationMat;

		SunData Sun;
		Vector3 AmbientColor;
		float NebulaIntensity;
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
		Vector2 FogFactors;
		float GammaBrightness;
	};
	double m_viewProjectLastD[16];
	Matrix m_viewProjectLast;

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
#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 || TRINITY_PLATFORM==TRINITY_OPENGL4 )
		Matrix ViewProjectionLast;
#endif

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
		float _;
		Vector2 ViewportSize;
	};

	struct PostProcessPSData
	{
		Matrix ReprojectionMatrix;
		Vector3 OriginShift;
		float DeltaT;
	};
	PostProcessPSData m_postProcessPSData;
	Tr2ShaderBufferPtr m_postProcessPSBuffer;
	Tr2ShaderBufferPtr GetPostProcessPSBuffer();
	void UpdatePostProcessPSData();

	void PopulatePerFrameVSData( PerFrameVSData &data );
	void PopulatePerFramePSData( PerFramePSData &data );
	void ApplyPerFrameData( Tr2RenderContext& renderContext );

	void GetShadowCasterRenderables( 
		IEveSpaceObject2* objectOfInterest,
		IEveShadowCaster* objectOfInterestShadowCaster,
		const std::vector<IEveShadowCaster*>& allShadowCasters,
		std::vector<ITr2Renderable*>& shadowRenderables,
		std::vector<IEveShadowCaster*>& debugShadowCasters );
	void PrepareShadowMap( 
		IEveSpaceObject2* objectOfInterest, 
		const std::vector<ITr2Renderable*>& shadowRenderables,
		const std::vector<IEveShadowCaster*>& debugShadowCasters,
		Tr2RenderContext& renderContext );
	void GetAllShadowCasters( std::vector<IEveShadowCaster*>& allShadowCasters );
	void EnableShadowRendering(bool _turnOn);

	void UpdatePlanets( EveUpdateContext& updateContext );
	void RenderPlanets( Tr2RenderContext& renderContext );

	void RenderDistortion( Tr2RenderContext& renderContext );

	Matrix SetupPlanetViewMatrix();

	void SetNoShadow();

	void GetPickingResults( Tr2PickBuffer& pickBuffer, Tr2RenderContext& renderContext, 
							unsigned short& objId, 
		                    unsigned short& areaId, float& depth );
	void DecodeBufferPixel( const void* pBuffer, unsigned short& objId, unsigned short& areaId,
		                    float& depth ) const;

	// Batch gathering and preparation	
	void GetAllBatchesFromRenderables( std::vector<ITr2Renderable*>& objectRenderables, Tr2RenderableSortList& objectsWithTransparencies, BatchMap& batches );
	void GetOpaqueBatchesFromRenderables( std::vector<ITr2Renderable*>& objectRenderables, BatchMap& batches );
	void GetDepthBatchesFromRenderables( std::vector<ITr2Renderable*>& objectRenderables, BatchMap& batches );
	void GetTransparentBatchesFromRenderables( std::vector<ITr2Renderable*>& objectRenderables, Tr2RenderableSortList& objectsWithTransparencies, BatchMap& batches );
	void PrepareTransparentBatch( Tr2RenderableSortList& objectsWithTransparencies, BatchMap& batches );
	
	// Batch rendering
	void RenderObjectsReceivingShadows( std::vector<ShadowReceiver>& objectReceivingShadows, bool renderShadows, Tr2RenderContext& renderContext );
	void RenderOpaqueBatches( BatchMap& batches, Tr2RenderContext& renderContext );
	void RenderTransparentBatches( BatchMap& batches, Tr2RenderContext& renderContext );
	void RenderDistortionBatches( BatchMap& batches, Tr2RenderContext& renderContext );

	// Utility rendering functions
	void RenderBatch(		ITriRenderBatchAccumulator* batch, 
							Tr2EffectStateManager::RenderingMode rm, 
							Tr2RenderContext &renderContext );

	void RenderRenderables( const std::vector<ITr2Renderable*> &renderables, 
							ITriRenderBatchAccumulator* batch, 
							TriBatchType batchType, 
							Tr2EffectStateManager::RenderingMode rm,
							Tr2RenderContext &renderContext );

	void UpdateSceneFromScript( Be::Time time );

protected:
	bool m_display;
	bool m_update;
	bool m_enableShadows;
	bool m_selfShadowOnly;
	bool m_displayShadowMap;
	bool m_backgroundRenderingEnabled;
	bool m_renderDebugInfo;
	bool m_debugShowShadowCasters;
	bool m_enableShadowObb;
	bool m_enableShadowDistanceTweak;
	unsigned int m_displayShadowMapMipLevel;
	float m_shadowFadeThreshold;
	float m_shadowThreshold;
	
	// To help avoid horrible performance in degenerate situations we
	// put a hard limit on the number of shadow maps drawn per frame.
	unsigned int m_shadowReceiverMaxCount;

	// To help avoid horrible performance in degenerate situations we
	// put a hard limit on the number of shadow casters drawn into
	// each shadow map.
	unsigned int m_shadowCasterMaxCount;
	float m_shadowCameraDistance;

	float m_planetScale;
	float m_planetCameraScale;

	// main shadow map
	TriShadowMapPtr m_shadowMap;

	PEveTransformVector		m_backgroundObjects;
	PEvePlanetVector		m_planets;
	PIEveSpaceObject2Vector m_objects;
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
	
	// Utility batches.
	ITriRenderBatchAccumulator* m_shadowBatches;
	ITriRenderBatchAccumulator* m_opaquePickingBatches;
	ITriRenderBatchAccumulator* m_pickingBatches;

	Tr2EffectPtr m_backgroundEffect;

	Quaternion m_envMapRotation;

	std::string m_envMapResPath;
	TriVariable* m_envMapHandle;
	TriTextureResPtr m_envMapTextureRes;

	std::string m_envMap1ResPath;
	TriTextureResPtr m_envMap1;

	std::string m_envMap2ResPath;
	TriTextureResPtr m_envMap2;

	std::string m_envMap3ResPath;
	TriTextureResPtr m_envMap3;

	Tr2Variable m_shadowLightnessVar;
	Tr2Variable m_envMap1Var;
	Tr2Variable m_envMap2Var;

	Tr2Variable m_reflectionMapVar;
	Tr2Variable m_reflectionMaskMapVar;

	Tr2Variable m_envMapTransformVar;
	Tr2Variable m_reflectionMapTransformVar;
	Tr2Variable m_suncVecVar;
		
	Tr2DepthStencilPtr m_depthMap;
	Tr2Variable m_depthMapVar;
	Tr2Variable m_depthMapMsaaVar;

	Tr2RenderTargetPtr m_distortionMap;
	Tr2RenderTargetPtr m_velocityMap;

	SunData m_sunData;
	Color m_sunColor;
	Color m_sunColorWithDynamicLights;
	bool m_useSunColorWithDynamicLights;

	Color m_ambientColor;
	Color m_fogColor;
	float m_nebulaIntensity;
	float m_fogStart; // Depth at which fogging starts
	float m_fogEnd; // Depth at which fog does not get stronger
	float m_fogMax; // Maximum strength of fog, range [0,1], at m_fogEnd distance.
	float m_fogType; // blend between static fog color and dynamic nebula background
	float m_fogBlur; // blur level of dynamic nebula background

	PerFramePSData m_perFramePS;
	PerFrameVSData m_perFrameVS;

	EveVisualizeMethod m_visualizeMethod;
	float m_perFrameDebug;

	EveTransformPtr m_dustfield;
	EveDustfieldConstraintPtr m_dustfieldConstaint;
	
	EveTransformPtr m_cloudfield;
	EveDustfieldConstraintPtr m_cloudfieldConstaint;

	PEveSceneStaticParticlesVector m_staticParticles;

	Tr2DataTextureManagerPtr m_dataTextureMgr;

	// For tracking the sunlight direction
	ITriVectorFunctionPtr m_sunBall;

	// ballpark of this scene
	IEveBallparkPtr m_ballpark;

	PTr2ExternalParameterVector m_externalParameters;

	Vector3 PickInfinity( int x, int y, Matrix proj, Matrix view );

	//For GPU particles
private:
	Tr2GpuParticleSystem* GetGpuParticleSystem() const
	{
		return m_updateContext.GetGpuParticleSystem();
	}
	void SetGpuParticleSystem( Tr2GpuParticleSystem* ps )
	{
		m_updateContext.SetGpuParticleSystem( ps );
	}

	Be::Time m_updateTime;
	EveSpaceObject2Ptr m_egoBall;

	Tr2ConstantBufferAL	m_perFrameVSBuffer;
	Tr2ConstantBufferAL	m_perFramePSBuffer;
	Tr2ConstantBufferAL	m_shadowPerFrameVSBuffer;

	// Picking
	void SetupTransformsForPicking( float fx, float fy, TriProjection* proj,  TriView* view, TriViewport* viewport, Tr2RenderContext& renderContext );
	void GetPickingObjectsToRender( std::vector<ITr2Renderable*>& pickableRenderObjects );

	Tr2PickBuffer m_pickBuffer;
	Tr2EffectPtr m_pickEffect;
	EveStarfieldPtr m_starfield;

	Tr2EffectPtr m_visualizerEffects[VM_COUNT];

	void UpdateVariableStore();
	void ClearVariableStore();

	void ClearBatches( BatchMap& batches );
	void FinalizeBatches( BatchMap& batches );

	virtual void ReleaseResources( TriStorage s );
	virtual bool OnPrepareResources();

	void UpdateShLighting( 
		const std::vector<ShadowReceiver>& objectsReceivingShadow, 
		const std::vector<IEveSpaceObject2*>& objectsNotReceivingShadow );

	void UpdateQuadRenderer( 
		const TriFrustum& frustum,
		const std::vector<ShadowReceiver>& objectsReceivingShadow, 
		const std::vector<IEveSpaceObject2*>& objectsNotReceivingShadow, 
		Tr2RenderContext& renderContext );

	Tr2QuadRenderer* GetQuadRenderer() const;

	Tr2ShLightingManagerPtr m_shLightingManager;

	TAASampling m_taaPattern;
	Vector2 m_taaSamplingPatterns[9];
	int m_taaSamplingIndex;
	float m_xProjOffset;
	float m_yProjOffset;

	float m_taaPixelOffsetScale;

	float m_nebulaBrightnessOverride;
	Tr2Variable m_nebulaBrightnessOverrideVar;
	void TAAOffset();
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
void GetBatchesFromRenderables(	ITr2Renderable** objectRenderables, const unsigned renderableCount, 
								Tr2RenderableSortList* objectsWithTransparencies, 
								EveSpaceScene::BatchMap& batches, 
								const TriBatchType* batchTypes, const unsigned batchTypeCount );


#endif // EveSpaceScene_H