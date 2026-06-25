// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EVESPACEOBJECTDECAL_H
#define EVESPACEOBJECTDECAL_H


#include "ITr2Renderable.h"
#include "Tr2PerObjectData.h"
#include "TriRenderBatch.h"
#include "Tr2DeviceResource.h"
#include "Tr2ShLightingManager.h"
#include "Tr2DebugRenderer.h"
#include "Eve/IEveSpaceObject2.h"
#include "Eve/EveUpdateContext.h"
#include "../../../Resources/TriGeometryRes.h"

BLUE_DECLARE_INTERFACE( ITr2InstanceData );
BLUE_DECLARE( Tr2Buffer );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE( TriVariable );
BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2DebugRenderer );
BLUE_DECLARE( Tr2InstancedMesh );

struct DecalVSPerObjectData
{
	Matrix m_worldMatrix;
	Matrix m_invWorldMatrix;
	Matrix m_decalMatrix;
	Matrix m_inverseDecalMatrix;
	Matrix m_parentBoneMatrix;
	Matrix m_invParentBoneMatrix;
};

struct DecalPSPerObjectData
{
	Vector4 m_displayData;
	Vector4 m_shipData;
	Vector4 m_clipData;
	float m_clipRadius2Sq;
	Vector3 m_unused;
	Vector4 m_shLightingCoefficients[Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT];
};

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
	void ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const override;

	// vs per object data
	DecalVSPerObjectData m_vsData;
	// pixel shader per object data
	DecalPSPerObjectData m_psData;
};

struct DecalMeshCache
{
	struct MeshBuffers
	{
		std::unique_ptr<uint8_t[]> vertexBuffer;
		std::unique_ptr<uint8_t[]> indexBuffer;
	};
	std::vector<MeshBuffers> buffers;
};

// --------------------------------------------------------------------------------
// Description:
//   ToDo
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSpaceObjectDecal ) :
	public IInitialize,
	public Tr2DeviceResource,
	public INotify,
	public ITr2Pickable,
	public ITr2Renderable
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EveSpaceObjectDecal( IRoot* lockobj = NULL );
	~EveSpaceObjectDecal();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * val );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );

private:
	bool OnPrepareResources();

public:
	//////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID( uint16_t )
	{
		return this->GetRawRoot();
	}
	virtual void GetPickingBatches( ITriRenderBatchAccumulator * batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual bool HasTransparentBatches();
	virtual void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator );

	// copy init
	void CopyFrom( EveSpaceObjectDecal * object );

	// access
	void UpdateVisibility( const EveUpdateContext& updateContext, const IEveSpaceObject2::ParentData* parentData );
	void GetRenderables( std::vector<ITr2Renderable*> & renderables, DecalMeshCache & meshCache, TriGeometryRes * geomRes, float screensize );
	void GetInstancedRenderables( std::vector<ITr2Renderable*> & renderables, DecalMeshCache & meshCache, const Tr2InstancedMesh* instancedMesh, float instanceScreenSize = std::numeric_limits<float>::max() );

	// access position etc.
	const Vector3& GetPosition() const;
	void SetPosition( const Vector3& pos );
	const Quaternion& GetRotation() const;
	void SetRotation( const Quaternion& rot );
	const Vector3& GetScaling() const;
	void SetScaling( const Vector3& sc );
	int GetBoneIndex() const;
	void SetBoneIndex( int idx );
	void SetIndices( const std::vector<std::vector<uint32_t>>& indices );
	void SetMinScreenSize( float minScreenSize );

	// edit helper
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& worldMatrix ) const;

	// set things from the parent, the spaceobject
	void SetBoneMatrix( const Float4x3* bonesMatrices, int bonesMatricesCount );
	void SetEffect( Tr2EffectPtr newEffect );

	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value );

	void SetHighDetailDecalState( bool isFrozen );

	void SetBatchType( TriBatchType batchType );

	void SetPriority( uint32_t priority );

private:
	// create
	void CreateDecalIndexBuffers( TriGeometryResPtr geomRes, DecalMeshCache & meshCache );
	// update
	void UpdateDecalMatrix();
	void CreateStaticIndexBuffers( TriGeometryResPtr geomRes );

	bool HasStaticIndexBuffers() const;
	std::vector<std::vector<uint32_t>> GetStaticIndexBuffers() const;
	std::vector<uint32_t> GetDecalPrimitiveCounts() const;

	// name
	std::string m_name;
	// display
	bool m_display;

	// parent ship data
	IEveSpaceObject2::ParentData m_parentData;

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
	Matrix m_inverseDecalMatrix;

	// base mesh geometry
	TriGeometryResPtr m_baseGeometryResource;
	int m_geometryLodIndex;

	std::shared_ptr<MeshDecalData> m_decalGeometry;
	std::vector<std::vector<uint32_t>> m_staticIndexBuffers;

	float m_isVisible;
	float m_minScreenSize;
	float m_instanceScreenSize;
	TriBatchType m_batchType;

	uint32_t m_priority;

	unsigned int m_vertexDeclarationOverride;
	ITr2InstanceData* m_instanceData;

	// Bounds for mimicing the frustum culling and fading for instanced hulls that have decals
	Vector3 m_minBounds, m_maxBounds;

	// the geometry can be frozen
	bool m_isGeometryFrozen;
};

TYPEDEF_BLUECLASS( EveSpaceObjectDecal );
BLUE_DECLARE_VECTOR( EveSpaceObjectDecal );

#endif
