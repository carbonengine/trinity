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
#include "Tr2GPUParticlePool.h"

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

BLUE_DECLARE( EveSpaceSceneLightMgr );
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

class EveSpaceScene :
	public ITr2Scene,
	public ITr2MultiPassScene,
	public IInitialize,
	public INotify,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	EveSpaceScene( IRoot* lockobj = NULL );
	~EveSpaceScene();

	static bool IsMeshUnloadingEnabled();

	IRoot* PickObject( int x, int y, TriProjection* proj,  TriView* view, TriViewport* viewport );	// for use by python, uses default immediate context
	IRoot* PickObjectAndArea( int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, unsigned int& areaID, Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2Scene
	virtual void Update( Be::Time time );
	virtual void Render( Tr2RenderContext& renderContext );
	virtual void RenderDebugInfo( Tr2RenderContext& renderContext );

	RenderPassResult RenderPass( PassType pass, Tr2RenderContext& renderContext );
	void RenderMainPass( Tr2RenderContext& renderContext );
	void RenderDepthPass( Tr2RenderContext& renderContext );
	bool RenderBackgroundPass( Tr2RenderContext& renderContext );
	void BeginRender( Tr2RenderContext& renderContext );
	void EndRender( Tr2RenderContext& renderContext );

	void GatherBatches( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();
	
	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

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

		/*

		VM_WHITE,

		VM_OBJECT_NORMAL,
		VM_SHADED_OBJECT_NORMAL,

		VM_TANGENT,
		VM_BITANGENT,

		VM_TEXCOORD0,
		VM_TEXCOORD1,

		VM_TEXELDENSITY0,

		VM_NORMALMAP,
		VM_NORMALMAP_UNPACKED,
		VM_NORMALMAP_RED_INVERTED,
		VM_NORMALMAP_GREEN_INVERTED,
		VM_NORMALMAP_BOTH_INVERTED,

		VM_SHADED_NORMALMAP,
		VM_SHADED_NORMALMAP_UNPACKED,
		VM_SHADED_NORMALMAP_RED_INVERTED,
		VM_SHADED_NORMALMAP_GREEN_INVERTED,
		VM_SHADED_NORMALMAP_BOTH_INVERTED,

		VM_DIFFUSEMAP,
		VM_GLOWMAP,
		VM_GLOWMAP_UNPACKED,
		VM_SPECULARMAP,
		VM_SPECULARMAP_UNPACKED,
		VM_REFLECTIONMAP,
		VM_AOMAP,
		VM_SHADOW_ONLY,

		VM_WIREFRAME,
		*/

		VM_COUNT
	};

	// BatchMaps are used to store batch accumulators for rendering
	typedef std::map<TriBatchType, ITriRenderBatchAccumulator*> BatchMap;

protected:
	// Data shared between the different rendering method chunks
	struct FrameData
	{
		Matrix projection;
		Matrix projectionDynamic;
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

	// This is the global ambient - including contributions from Light, Sun and D3D global ambient
	struct SceneData
	{
		Color AmbientColor;
		Color FogColor; 
	};

	// Per-frame pixel constants for rendering scene
	struct PerFramePSData
	{
		Matrix ViewInverseTransposeMat;
		Matrix ProjectionMat;
		Matrix ViewMat;
		Matrix EnvMapRotationMat;

		SunData Sun;
		SceneData Scene;

		Vector2 CameraRange;
		Vector2 ViewportOffset;
		// pass resolution to pixelshader, can be usefull in some crazy shaders
		Vector2 TargetResolution;
		// pass fov x and y
		Vector2 FovXY;

		Vector4 ShadowMapSettings;
		float Time;
		float ShadowLightness;
		Vector2 ViewportSize;
#if TRINITY_PLATFORM == TRINITY_DIRECTX11
		float DepthMapSampleCount;
		Vector3 unused_;
#endif
	};

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
		// scene color info
		SceneData Scene;

		float Time;
		float _;
		Vector2 ViewportSize;
	};

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

	// lighting
	void PrepareLighting();

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

	// main shadow map
	TriShadowMapPtr m_shadowMap;

	PEveTransformVector		m_backgroundObjects;
	PEvePlanetVector		m_planets;
	PIEveSpaceObject2Vector m_objects;
	IEveSpaceObject2Ptr		m_warpTunnel;
	PTriCurveSetVector		m_curveSets;
	PEveLensflareVector		m_lensflares;
	EveUpdateContext		m_updateContext;

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

	// lights manager of this scene
	EveSpaceSceneLightMgrPtr m_sceneLightMgr;

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

	SunData m_sunData;
	SceneData m_sceneData;

	float m_fogStart; // Depth at which fogging starts
	float m_fogEnd; // Depth at which fog does not get stronger
	float m_fogMax; // Maximum strength of fog, range [0,1], at m_fogEnd distance.

	PerFramePSData m_perFramePS;
	PerFrameVSData m_perFrameVS;

	EveVisualizeMethod m_visualizeMethod;
	bool m_isWireframe;

	EveTransformPtr m_dustfield;
	EveDustfieldConstraintPtr m_dustfieldConstaint;

	// For tracking the sunlight direction
	ITriVectorFunctionPtr m_sunBall;

	// ballpark of this scene
	IEveBallparkPtr m_ballpark;

	Vector3 PickInfinity( int x, int y, Matrix proj, Matrix view );

	//For GPU particles
private:
	void UpdateEgoPosition( double x, double y, double z );

	Tr2GPUParticlePoolManager* GetParticlePoolManager(){ return m_updateContext.GetParticlePoolManager();}
	void SetParticlePoolManager(Tr2GPUParticlePoolManager* manager){ return m_updateContext.SetParticlePoolManager(manager);}
	void UpdateStatefulParticles( Be::Time time, const Vector3 &, Tr2RenderContext &, Tr2GPUParticlePoolManager* manager );
	void RenderStatefulParticles( const Tr2GPUParticleRenderMode mode, Tr2RenderContext& context, Tr2GPUParticlePoolManager* manager );

	bool m_egoPositionInit;
	Vector3d m_egoPosition;
	Vector3 m_egoDisplacement;
	Be::Time m_updateTime;
	EveSpaceObject2Ptr m_egoBall;

private:
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

	bool m_dynamicClipPlanes;
	float m_nearClip;
	float m_farClip;
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