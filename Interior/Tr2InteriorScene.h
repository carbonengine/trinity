#pragma once
#ifndef Tr2InteriorScene_H
#define Tr2InteriorScene_H


#include "ITr2PickableScene.h"
#include "ITr2VisibilityQueryable.h"
#include "Tr2PickBuffer.h"
#include "Tr2InteriorVisualization.h"
#include "include/ITr2MultiPassScene.h"
#include "ITr2VisualizationModeRenderer.h"
#include "Tr2InteriorLightSet.h"
#include "Include/ITr2Scene.h"
#include "Tr2InteriorRenderBatch.h"
#include "Include/ITr2Interior.h"

// Forward declarations
struct Tr2VisibilityEvent;
struct Tr2PerFramePSData;
struct Tr2PerFrameVSData;
class TriProjection;
class TriVariable;
class TriView;
class TriViewport;

// Blue forward declarations
BLUE_DECLARE( Tr2ApexScene );
BLUE_DECLARE( Tr2InteriorScene );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2VisibilityResults );
BLUE_DECLARE( Tr2TextureAtlas );
BLUE_DECLARE_VECTOR( Tr2TextureAtlas );
BLUE_DECLARE_INTERFACE( ITr2PhysicsUpdater );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( TriLineSet );

class Tr2InteriorScene:
	public IInitialize,
	public INotify,
	public IListNotify,
	public ITr2Scene,
	public ITr2MultiPassScene,
	public ITr2PickableScene,
	public ITr2VisibilityQueryable,
	public Tr2DeviceResource,
	public ITr2VisualizationModeRenderer
{
public:
	Tr2InteriorScene( IRoot* lockobj = NULL );
	~Tr2InteriorScene();

	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
    virtual bool OnModified( Be::Var* value );
	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );

	//////////////////////////////////////////////////////////////////////////
	// ITr2Scene
	virtual void Update( Be::Time realTime, Be::Time simTime );
	virtual void Render( Tr2RenderContext& renderContext );
	virtual void RenderDebugInfo( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////
	// ITr2MultiPassScene
	virtual RenderPassResult RenderPass( PassType pass, Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////
	// ITr2VisibilityQueryable
	virtual void VisibilityQuery( Tr2VisibilityResults* visibilityResults );
	virtual void SetVisibilityResults( Tr2VisibilityResults* visibilityResults );

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:	

	//////////////////////////////////////////////////////////////////////////
	// ITr2VisualizationModeRenderer
	virtual void SetVisualizationMode( int visualizationMode );

	void SetRenderBackgroundCubeMap( bool renderBackgroundCubemap );

	void RebuildSceneData( void );

	// Add a light source to the scene
	void AddLightSource( ITr2InteriorLight* lightSource );
	// Remove a light source from the scene
	void RemoveLightSource( ITr2InteriorLight* lightSource );
	// Add a dynamic to the scene
	void AddDynamic( ITr2InteriorDynamic* dynamic );
	// Remove a dynamic from the scene
	void RemoveDynamic( ITr2InteriorDynamic* dynamic );

protected:

	void OnQueryBegin( void );
	void OnInstanceVisible( ITr2InteriorCullable* cullable, const Matrix& );

	//////////////////////////////////////////////////////////////////////////
	// Visibility event handlers

	// Handle Tr2VisibilityEvent::QUERY_BEGIN
	void DoQueryBegin( const Tr2VisibilityEvent& event );
	// Handle Tr2VisibilityEvent::INSTANCE_VISIBLE
	void DoInstanceVisible( const Tr2VisibilityEvent& event );

	// Gather batches for full-forward pass (not prepass)
	void GatherFullForwardBatches( Tr2VisibilityResults* results );

private:
	void ResolveVisibility( const Matrix& view, const Matrix& projection, size_t maxDepth );

	void RenderFullForward( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////
	// Rendering utility functions

	// Queue up a background cubemap batch
	void PrepareBackgroundCubemapBatch( ITriRenderBatchAccumulator* batches );

private:

	// Holds the main render batch list for opaques, decals, and transparent batches
	ITriRenderBatchAccumulator* m_primaryRenderBatches;
	// Holds the opaque batches for pickable objects during a picking operation
	ITriRenderBatchAccumulator* m_opaquePickingBatches;
	// Holds the opaque batches for pickable objects during a picking operation
	ITriRenderBatchAccumulator* m_pickingBatches;

	// Direction and color of sun light
	Vector3 m_sunDirection;
	Color m_sunDiffuseColor;
	Color m_sunSpecularColor;

	// Handle to "Sun.DirWorld" and "Sun.DiffuseColor" variables in variable store
	Tr2Variable m_sunDirectionVar;
	Tr2Variable m_sunDiffuseColorVar;
	Tr2Variable m_sunSpecularColorVar;

	Color m_ambientColor;
	Tr2Variable m_ambientColorVar;

	Tr2Variable m_cameraPosVar;

	// lights
	PITr2InteriorLightVector m_lights;
	// dynamics
	PITr2InteriorDynamicVector m_dynamics;
	// dynamics pending load
	PITr2InteriorDynamicVector m_dynamicsPendingLoad;

	// Visibility result set
	Tr2VisibilityResultsPtr m_visibilityResults;

	enum VisibilityQueryType
	{
		PRIMARY_QUERY,
		PICKING_QUERY
	};
	VisibilityQueryType m_visibilityQueryType;

	// Active light set
	Tr2InteriorLightSet m_activeLightSet;

	// Maintain a list of visible objects (used for picking)
	std::vector<ITr2Renderable*> m_visibleObjects;

	Tr2Variable m_backgroundCubeMapVar;
	std::string m_backgroundCubeMapPath;
	TriTextureResPtr m_backgroundCubeMapRes;
	Tr2EffectPtr m_backgroundEffect;
	bool m_renderBackgroundCubeMap;

	Tr2ApexScenePtr	m_apexScene;

	void SetBackgroundCubemapResPath();

	void RenderGeometry( Tr2Material* overrideEffect, Tr2RenderContext& renderContext );

	void SetupTransformsForPicking( float fx, float fy, TriProjection* proj, TriView* view, TriViewport* viewport );
	const std::vector<ITr2Renderable*>& GetPickingObjectsToRender( const Vector3& dirWorld );
	const std::vector<ITr2Renderable*>& GetPickingObjectsToRender( const Vector3& dirWorld, float fov, float aspect );

	virtual void SetPerFrameDataForPicking( void );

	virtual ITriRenderBatchAccumulator* GetOpaquePickingBatchAccumulator();
	virtual ITriRenderBatchAccumulator* GetPickingBatchAccumulator();

	virtual Tr2Material* GetPickingEffect( PickComponents pass );
	virtual bool RenderPickingAreasForComponents( PickComponents pass ) const { return true; }
	virtual Tr2PickBuffer& GetPickBuffer( void ) { return m_pickBuffer; }
	virtual unsigned int GetRequiredPasses( PickComponents requestedComponents, PickComponents* passes );
	virtual void DecodeBufferPixel( const void* pBuffer, PickComponents pass, BufferResults& results ) const;

	// Construct a sort key from object & batch groups
    int64_t ConstructKey( unsigned int objectGroup, Tr2InteriorBatchGroup batchGroup );

	// These should be moved over to a smaller per-frame data at some point without the cruft
	void PopulatePerFramePSData( Tr2PerFramePSData &data );
	void PopulatePerFrameVSData( Tr2PerFrameVSData &data );

	// This is a python only wrapper function for just picking an object
	IRoot* PickObjectOnly( int x, int y, TriProjection* proj, TriView* view, TriViewport* vp, Be::Optional<Tr2PickTypes> )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		PickResults results;
		results.components = PICK_OBJECT;
		PickObject( renderContext, x, y, proj, view, vp, results );
		return results.object;
	}

	Vector2 PickObjectUV( int x, int y, TriProjection* proj, TriView* view, TriViewport* vp )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		PickResults results;
		results.components = PICK_UV;
		PickObject( renderContext, x, y, proj, view, vp, results );
		return results.uv;
	}

	PTriCurveSetVector m_curveSets;

	// Per-frame data
	Tr2PerFrameVSData m_perFrameVSData;
	Tr2PerFramePSData m_perFramePSData;

	// Mouse picking
	Tr2PickBuffer m_pickBuffer;
	Tr2EffectPtr m_pickEffect;

	Be::Time m_lastUpdateTime;

	float m_apexLODResourceBudget;
	float m_apexLODResourceBudgetConsumed;

	// debug
	bool m_renderDebugInfo;
    TriLineSetPtr m_debugLines;

	// visualization
	VisualizeMethod m_visualizeMethod;

	// Ragdoll simulation
	ITr2PhysicsUpdaterPtr m_ragdollScene;

	// N dot L lookup texture (used during lighting pass)
	TriTextureResPtr m_nDotLTexture;
	// N dot L lookup texture variable handle (used during lighting pass)
	TriVariable* m_nDotLTextureHandle;

	// Maximum fog density amount (from 0 to 1)
	float m_maxFogAmount;
	// Distance where fog reaches maximum density
	float m_maxFogDistance;
	// Distance where fog density starts to grow from 0
	float m_minFogDistance;
	// Fog color
	Color m_fogColor;

	Tr2ConstantBufferAL	m_perFramePSBuffer;
	Tr2ConstantBufferAL	m_perFrameVSBuffer;


};

TYPEDEF_BLUECLASS( Tr2InteriorScene );

#endif
