#pragma once
#ifndef EVESPACEOBJECTDECAL_H
#define EVESPACEOBJECTDECAL_H


#include "ITr2GeometryProvider.h"
#include "ITr2Renderable.h"
#include "Tr2PerObjectData.h"
#include "TriRenderBatch.h"
#include "Tr2DeviceResource.h"

#include "Eve/SpaceObject/EveShip2.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE( TriVariable );
BLUE_DECLARE( TriFrustum );

// --------------------------------------------------------------------------------
// Description:
//   This class holds the per object data for decals
// SeeAlso:
//   Tr2PerObjectData
// --------------------------------------------------------------------------------
class EveDecalPerObjectData : public Tr2PerObjectData
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;

	// vs per object data
	Matrix m_worldMatrix;
	Matrix m_invWorldMatrix;
	Matrix m_decalMatrix;
	Matrix m_invDecalMatrix;
	Matrix m_parentBoneMatrix;
	// pixel shader per object data
	// empty for now...
};

// --------------------------------------------------------------------------------
// Description:
//   This class stores mesh vertex and index buffer data to speed up decal 
//   construction when a space object has several decals and either vertex or index
//   buffer of a mesh is in slow access memory (may happen with extended D3D 
//   device).
// --------------------------------------------------------------------------------
class EveSpaceObjectDecalCache
{
public:
	EveSpaceObjectDecalCache();
	~EveSpaceObjectDecalCache();

	void Clear();
private:
	// Vertex buffer mirror data
	unsigned char* m_vertices;
	// Index buffer mirror data
	unsigned char* m_indices;
	friend class EveSpaceObjectDecal;
};

// --------------------------------------------------------------------------------
// Description:
//   ToDo
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSpaceObjectDecal ) : 
	public IInitialize,
	public ITr2GeometryProvider,
	public Tr2DeviceResource,
	public INotify,
	public ITr2Renderable
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EveSpaceObjectDecal(IRoot* lockobj = NULL);
	~EveSpaceObjectDecal();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	virtual void SubmitGeometry( Tr2RenderContext& renderContext );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual bool HasTransparentBatches();
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	// access
	void GetRenderables( TriGeometryResPtr geomRes, const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix* worldMatrix );

	// access position etc.
	const Vector3& GetPosition() const;
	void SetPosition( const Vector3& pos );
	const Quaternion& GetRotation() const;
	void SetRotation( const Quaternion& rot );
	const Vector3& GetScaling() const;
	void SetScaling( const Vector3& sc );

	// edit helper
	void RenderDebugInfo( const Matrix* worldMatrix ) const;

	// set things from the parent, the spaceobject
	void SetCache( EveSpaceObjectDecalCache* cache );
	void SetBoneMatrix( const granny_matrix_3x4* bonesMatrices, int bonesMatricesCount );
	void SetEffect( Tr2EffectPtr newEffect );

private:
	// create
	void CreateDecalIndexBuffer( TriGeometryResPtr geomRes );
	// update
	void UpdateDecalMatrix();

	// name
	std::string m_name;
	// display
	bool m_display;
	bool m_displayBoundingBox;

	// parent matrix
	Matrix m_parentTransform;

	// decal shader
	Tr2EffectPtr m_decalEffect;

	// orientation data of the decal projection
	Vector3 m_position;
	Quaternion m_rotation;
	Vector3 m_scaling;

	// decals can be parented to bones
	int m_parentBoneIndex;
	Matrix m_parentBoneMatrix;

	// matrices representing the "volume" of the decal projection
	Matrix m_decalMatrix;
	Matrix m_invDecalMatrix;

	// base mesh geometry
	TriGeometryResPtr m_baseGeometryResource;

	// new index buffer
	Tr2IndexBufferAL m_indexBuffer;
	bool m_rebuildIndexBuffer;
	// num of primitives for this decal
	unsigned int m_decalPrimitiveCount;

	// Shared mesh data cache
	EveSpaceObjectDecalCache* m_cache;
};

TYPEDEF_BLUECLASS( EveSpaceObjectDecal );
BLUE_DECLARE_VECTOR( EveSpaceObjectDecal );

#endif
