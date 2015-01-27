#pragma once
#ifndef Tr2InteriorStatic_h
#define Tr2InteriorStatic_h

#include "include/ITr2Interior.h"

#include "Tr2InteriorLightSet.h"
#include "Tr2InteriorVisualization.h"
#include "Utilities/BoundingBox.h"

class TriGeometryRes;
class Tr2InteriorMirror;
class Tr2PerObjectData;

BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE_VECTOR( Tr2Mesh );
BLUE_DECLARE( Tr2InteriorCell );
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
    // ITr2InteriorCullable
	virtual bool IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const;

    //////////////////////////////////////////////////////////////////////////
    // ITr2Interior
    virtual void RemoveFromCell();
	virtual const std::string& GetName( void ) const { return m_name; }
	virtual void SetSHLightingSolver( ITr2InteriorSHLightingSolver* solver ) { m_shSolver = solver; }

	// Set the parent cell with a weak reference, used to track cell visibility
	void SetParentCell( Tr2InteriorCell* parentCell );

	// Per-object data for instanced lighting
	virtual Tr2PerObjectData* GetPerObjectDataWithPerInstanceLighting( 
								ITriRenderBatchAccumulator* accumulator,
								Tr2InteriorLightSet* lightSet, 
								const Matrix& objectToWorldMatrix, 
								const Matrix& mirrorToWorldMatrix );
public:	

#if TRINITYDEV
	virtual void GetDescription( std::string& desc ) { desc = "Tr2InteriorStatic"; }
#endif

	// Set the visualization method on the scene being rendered so we can decide
	// more specifically what to rneder on the static (target vs detail, for example)
	void SetVisualizeMethod( VisualizeMethod method );
	// Get local bounding box
	bool GetBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const;
	// Get world bounding box
	bool GetWorldBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const;
	const Matrix& GetTransform() const { return m_transform; }
	Matrix GetWorldTransform() const;
	TriGeometryRes* GetGeometryResource() const;
	std::string GetGeometryResourcePath() const;
	// Gets the dirty flag
	bool IsDirty( void ) const;
	// Resets the dirty flag
	void ResetDirtyFlag( void );

	void SetInstanceData( const Vector4 &linearTransform, const Vector2 &translation, unsigned int instanceInSystemIdx );

	// timing
	void Update( Be::Time time );

	void BindLowLevelShaders();
protected:
    CullResult AddToCell();

	Tr2PerObjectData* GetPerObjectDataWithLightSet( ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet, const Matrix& objectToWorldMatrix, const Matrix& mirrorToWorldMatrix );

	// Helper function for opaque detail mesh batches, used by GetBatches
	void GetDetailMeshOpaqueBatches( 
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

	// display
	bool m_display;

	// position & orientation
	Vector3 m_position;
	Quaternion m_rotation;
	Matrix m_transform;
	AxisAlignedBoundingBox m_worldBoundingBox;

	// Dirty flag
	bool m_isDirty;

	Vector4 m_uvLinearTransform;
	Vector2 m_uvTranslation;

	// the underlying geometry
	void InitializeGeometryResource();
	PTr2MeshVector m_detailMeshes;
	TriGeometryResPtr m_geometryResource;

	Tr2InteriorCell* m_parentCell;

	// Current visualize override method
	VisualizeMethod m_visualizeMethod;

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
