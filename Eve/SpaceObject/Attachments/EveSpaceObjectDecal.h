#pragma once
#ifndef EVESPACEOBJECTDECAL_H
#define EVESPACEOBJECTDECAL_H


#include "ITr2GeometryProvider.h"
#include "ITr2Renderable.h"
#include "Tr2PerObjectData.h"
#include "TriRenderBatch.h"
#include "Tr2DeviceResource.h"
#include "Tr2ShLightingManager.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE( TriVariable );
BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2DebugRenderer );

typedef uint32_t EveSpaceObjectDecalIndex;
BLUE_DECLARE_STRUCTURE_LIST( EveSpaceObjectDecalIndex );

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
	Matrix m_invParentBoneMatrix;
	// pixel shader per object data
	Vector4 m_displayData;
	Vector4 m_shipData;
	Vector4 m_clipData1;
	Vector4 m_clipData2;
	Vector4 m_shLightingCoefficients[Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT];
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
	public ITr2Pickable,
	public ITr2Renderable
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EveSpaceObjectDecal(IRoot* lockobj = NULL);
	~EveSpaceObjectDecal();

	//////////////////////////////////////////////////////////////////////////////////////
	// public structs
	struct ParentData
	{
		Matrix transform;
		uint32_t displayCounter;
		Vector4 shipData;
		Vector4 clipData;
		Vector4 clipDataEx;
		const Vector4* shLighting;
	};

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

	//////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID( uint16_t ) { return this->GetRawRoot(); }
	virtual void GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	virtual void SubmitGeometry( Tr2RenderContext& renderContext );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual bool HasTransparentBatches();
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	// copy init
	void CopyFrom( EveSpaceObjectDecal *object );

	// access
	void UpdateVisibility( const TriFrustum& frustum, const ParentData* parentData );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables, TriGeometryRes* geomRes );

	// access position etc.
	const Vector3& GetPosition() const;
	void SetPosition( const Vector3& pos );
	const Quaternion& GetRotation() const;
	void SetRotation( const Quaternion& rot );
	const Vector3& GetScaling() const;
	void SetScaling( const Vector3& sc );
	int GetBoneIndex() const;
	void SetBoneIndex( int idx );
	void SetIndices( const uint32_t* indices, size_t count );
	void SetMinScreenSize( float minScreenSize );

	// edit helper
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix ) const;

	// set things from the parent, the spaceobject
	void SetBoneMatrix( const granny_matrix_3x4* bonesMatrices, int bonesMatricesCount );
	void SetEffect( Tr2EffectPtr newEffect );

	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value );

private:
	// create
	void CreateDecalIndexBuffer( TriGeometryResPtr geomRes );
	// update
	void UpdateDecalMatrix();
	void CreateStaticIndexBuffer();
	bool HasStaticIndexBuffer() const;
	std::vector<EveSpaceObjectDecalIndex> GetStaticIndexBuffer();

	// name
	std::string m_name;
	// display
	bool m_display;

	// parent ship data
	ParentData m_parentData;

	// decal shader
	Tr2EffectPtr m_decalEffect;

	// orientation data of the decal projection
	Vector3 m_position;
	Quaternion m_rotation;
	Vector3 m_scaling;

	// decals can be parented to bones
	int32_t m_parentBoneIndex;
	Matrix m_parentBoneMatrix;
	Matrix m_invParentBoneMatrix;

	// matrices representing the "volume" of the decal projection
	Matrix m_decalMatrix;
	Matrix m_invDecalMatrix;

	// base mesh geometry
	TriGeometryResPtr m_baseGeometryResource;

	// new index buffer
	Tr2BufferAL m_indexBuffer;
	bool m_rebuildIndexBuffer;
	float m_isVisible;
	// num of primitives for this decal
	unsigned int m_decalPrimitiveCount;
	float m_minScreenSize;

	PEveSpaceObjectDecalIndexStructureList m_indices;
};

TYPEDEF_BLUECLASS( EveSpaceObjectDecal );
BLUE_DECLARE_VECTOR( EveSpaceObjectDecal );

#endif
