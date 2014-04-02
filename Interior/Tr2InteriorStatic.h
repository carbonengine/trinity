#pragma once
#ifndef Tr2InteriorStatic_h
#define Tr2InteriorStatic_h

#include "include/ITr2Interior.h"

#include "Tr2InteriorLightSet.h"
#include "Tr2InteriorVisualization.h"

//--------------------------------------------------------------------------------------------------
// forwards
//
namespace Umbra
{
	class Object;
	class Model;
}

class TriGeometryRes;
class Tr2InteriorMirror;
class Tr2PerObjectData;

BLUE_DECLARE( Tr2EnlightenArea );
BLUE_DECLARE_VECTOR( Tr2EnlightenArea );
BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE_VECTOR( Tr2Mesh );
BLUE_DECLARE( Tr2InteriorEnlightenSystem );
BLUE_DECLARE( TriLineSet );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( Tr2ShaderMaterial );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( TriGrannyRes );

//--------------------------------------------------------------------------------------------------
// wod interior static
//
BLUE_DECLARE( Tr2InteriorStatic );

class Tr2InteriorStatic:
	public ITr2Interior,
	public ITr2Renderable,
	public IInitialize,
	public INotify,
	public ITr2Pickable,
	public IBlueAsyncResNotifyTarget,
	public Tr2DeviceResource
{
public:
	Tr2InteriorStatic( IRoot* lockobj = NULL );
	~Tr2InteriorStatic();

	EXPOSE_TO_BLUE();

	using ITr2Interior::Lock;
	using ITr2Interior::Unlock;

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	//////////////////////////////////////////////////////////////////////////
	// IAsyncLoadedResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* p );
	void RebuildCachedData( BlueAsyncRes* p );

	//////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID() { return this->GetRawRoot(); }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, 
							 TriBatchType batchType, 
							 const Tr2PerObjectData* perObjectData );

	virtual bool HasTransparentBatches();
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

    //////////////////////////////////////////////////////////////////////////
    // ITr2Interior
	virtual void SetVisibility( bool bVisible );
	virtual bool IsVisible( void ) const { return m_isVisible; }
    virtual void RemoveFromCell();
    virtual CullResult AddToCell( Umbra::Cell* cell );
	virtual const std::string& GetName( void ) const { return m_name; }
	virtual void SetSHLightingSolver( ITr2InteriorSHLightingSolver* solver ) { m_shSolver = solver; }
	virtual bool CastsShadows() const { return true; }

	// Set the parent cell with a weak reference, used to track cell visibility
	void SetParentSystem( Tr2InteriorEnlightenSystem* parentSystem );

	// Per-object data for instanced lighting
	virtual Tr2PerObjectData* GetPerObjectDataWithPerInstanceLighting( 
								ITriRenderBatchAccumulator* accumulator,
								Tr2InteriorLightSet* lightSet, 
								const Matrix& objectToWorldMatrix, 
								const Matrix& mirrorToWorldMatrix );

	// Per-object data for pre-pass
	virtual Tr2PerObjectData* GetPerObjectDataForPrePass(
		ITriRenderBatchAccumulator* accumulator,
		const Matrix& objectToWorldMatrix
	);

	// Set mirror depth
	virtual void SetMirrorDepth( int depth ) {}
public:	

#if TRINITYDEV
	virtual void GetDescription( std::string& desc ) { desc = "Tr2InteriorStatic"; }
#endif

	// Set the visualization method on the scene being rendered so we can decide
	// more specifically what to rneder on the static (target vs detail, for example)
	void SetVisualizeMethod( VisualizeMethod method );

	// Sets the transform needed to look up the illumination of this static within
	// the enlighten system textures
	void SetEnlightenInstanceTransform( float linearTransform[2][2], 
										float translation[2]);
	// Sets an id value for the instance, to keep the material ids in a low range
	void SetInstanceInSystemIdx( unsigned int i ) { m_instanceInSystemIdx = i; }
	// Getter for the instance id that was set when the static was built into the
	// system
	unsigned int GetInstanceInSystemIdx() { return m_instanceInSystemIdx; }
	// Get local bounding box
	bool GetBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const;
	// Get world bounding box
	bool GetWorldBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const;
	const Matrix& GetTransform() const { return m_transform; }
	const Matrix& GetParentTransform() const;
	Matrix GetWorldTransform() const;
	const PTr2EnlightenAreaVector& GetEnlightenAreas() const;
	TriGeometryRes* GetGeometryResource() const;
	std::string GetGeometryResourcePath() const;
	// Gets the dirty flag
	bool IsDirty( void ) const;
	// Resets the dirty flag
	void ResetDirtyFlag( void );

	void SetInstanceData( const Vector4 &linearTransform, const Vector2 &translation, unsigned int instanceInSystemIdx );

	// timing
	void Update( Be::Time time );

	// Light counting
	virtual void SetVisibleLightCount( int visibleLightCount );
	void SetTotalLightCount( int totalLightCount );

	// debug
	void RenderDebugInfo( TriLineSetPtr lines ) const;
	virtual void SetVisibleLightSet( const Tr2InteriorLightSet& visibleLightSet );

	void BindLowLevelShaders();

	bool CanRenderEnlightenMaterialTextures() const;
	void RenderEnlightenMaterialTexture( const char* situationName, Tr2RenderContext &renderContext );

	void RenderOcclusionGeometry();
protected:
	Tr2PerObjectData* GetPerObjectDataWithLightSet( ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet, const Matrix& objectToWorldMatrix, const Matrix& mirrorToWorldMatrix );

	// Helper function for opaque target mesh batches, used by GetBatches 
	void GetEnlightenTargetMeshOpaqueBatches( 
		ITriRenderBatchAccumulator* batches, 
		const Tr2PerObjectData* perObjectData );

	// Helper function for opaque detail mesh batches, used by GetBatches
	void GetEnlightenDetailMeshOpaqueBatches( 
		ITriRenderBatchAccumulator* batches, 
		const Tr2PerObjectData* perObjectData,
		TriBatchType batchType );

	// Helper function for getting transparent batches (with depth)
	void GetTransparentBatches( ITriRenderBatchAccumulator* batches, 
		                        Tr2Mesh* mesh,
								const Tr2PerObjectData* data );

	virtual void ReleaseResources( TriStorage s );
	virtual bool OnPrepareResources();

protected:
	// name
	std::string m_name;
	std::string m_geometryResPath;
	std::string m_occlusionResPath;

	// display
	bool m_display;
	bool m_displayTargetMesh;
	bool m_displayDetailMeshes;

	// position & orientation
	Vector3 m_position;
	Quaternion m_rotation;
	Matrix m_transform;

	// Dirty flag
	bool m_isDirty;

	Vector4 m_uvLinearTransform;
	Vector2 m_uvTranslation;

	unsigned int m_instanceInSystemIdx;

	// the underlying geometry
	void InitializeGeometryResource();
	PTr2EnlightenAreaVector m_enlightenAreas;
	PTr2MeshVector m_detailMeshes;
	TriGeometryResPtr m_geometryResource;

	Tr2ShaderMaterialPtr m_enlightenAreaMaterial;

	// Occlusion geometry
	void InitializeOcclusionResource();
	void BuildOcclusionGeometry();
	void DestroyOccluders();
	void RebuildOcclusionObjects();
	void DestroyOcclusionObjects();
	TriGrannyResPtr m_occlusionResource;
	// GeometryRes constructed (on demand) from m_occlusionResource.
	// Used only for VM_OCCLUSION visualization.
	TriGeometryResPtr m_occlusionDebugRenderResource;
	struct Occluder
	{
		Umbra::Object* m_object;
		Umbra::Model* m_model;
	};
	std::vector<Occluder> m_occluders;
	bool m_isOcclusionGeometryReady;
	bool m_useOcclusionGeometry;

	// lightsources on this static
	Tr2InteriorLightSet m_lightSet;

	// Pointer to the parent system
	Tr2InteriorEnlightenSystem* m_parentSystem;

	// Current visualize override method
	VisualizeMethod m_visualizeMethod;

	// culling
	bool m_isVisible;
	Umbra::Object* m_umbraObject;
	Umbra::Model* m_umbraModel;

	// visible lights
	int m_visibleLightCount;
	int m_totalLightCount;

	// Draw light spider for debugging
	bool m_drawLightSpider;
	std::vector<Vector3> m_visibleLights;
	// SH lighting solver for transparent rendering
	ITr2InteriorSHLightingSolver *m_shSolver;

	PTriCurveSetVector m_curveSets;

	// Depth offset for transparency sorting
	float m_depthOffset;

	// Cached mirror-to-world matrix from last GetPerObjectData call
	Matrix m_mirrorToWorldMatrix;

	// used by SetPerObjectDataToDevice
	mutable Tr2ConstantBufferAL	m_perObjectConstantBuffers[ Tr2RenderContextEnum::SHADER_TYPE_COUNT ];
};
TYPEDEF_BLUECLASS( Tr2InteriorStatic );
BLUE_DECLARE_VECTOR( Tr2InteriorStatic );

#endif
