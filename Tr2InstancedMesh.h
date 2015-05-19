////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef Tr2InstancedMesh_H
#define Tr2InstancedMesh_H

#include "Tr2Mesh.h"
#include "Tr2DeviceResource.h"

BLUE_DECLARE_INTERFACE( ITr2InstanceData );
BLUE_DECLARE( Tr2InstancedMesh );
BLUE_DECLARE_INTERFACE( ITr2GpuBuffer );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InstancedMesh is a specialization of Tr2Mesh that renders instanced geometry.
//   Instance data is provided via a TriGeometryRes object, which vertex buffer is used
//   as instance data.
// See Also:
//   Tr2Mesh
// --------------------------------------------------------------------------------------
class Tr2InstancedMesh:
	public Tr2Mesh,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	Tr2InstancedMesh( IRoot* lockobj = NULL );
	~Tr2InstancedMesh();

	using Tr2Mesh::Lock;
	using Tr2Mesh::Unlock;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:	
#if TRINITYDEV
	void GetDescription( std::string& desc ) { desc = "Tr2InstancedMesh"; }
#endif

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// IAsyncLoadedResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* p );
	void RebuildCachedData( BlueAsyncRes* p );

	/////////////////////////////////////////////////////////////////////////////////////
	// Tr2Mesh
	const char* GetInstanceMeshResPath() { return m_instanceGeometryResPath.c_str(); }
	void SetInstanceMeshResPath( const char* path ) { m_instanceGeometryResPath = path; }
	int GetInstanceMeshIndex() const { return m_instanceMeshIndex; };

	ITr2InstanceData* GetInstanceGeometryResource() const;
	void SetInstanceGeometryRes( ITr2InstanceData* res );

	void GetBatches( ITriRenderBatchAccumulator* batches,
					 const Tr2MeshAreaVector* areas, 
					 const Tr2PerObjectData* data,
					 ITr2MeshBatchCallback* callback = nullptr ) const;
	float CalcMeshSortValue( const Matrix& worldTransform );
	bool GetBoundingBox( Vector3& min, Vector3& max ) const;
	void SetBoundingBox( const Vector3& min, const Vector3& max );
	bool GetAreaBoundingBox( unsigned int areaIx, Vector3& min, Vector3& max ) const;
	bool GetBoundingSphere( Vector4& sphere );
	bool IsLoading() const;
protected:
	class Batch;

	void RenderAreas( unsigned int areaIx, unsigned int areaCount, bool reversed, Tr2RenderContext& renderContext );
private:
	// ----------------------------------------------------------------------------------
	// Description:
	//   A key structure for mapping mesh area to indirect buffer.
	// ----------------------------------------------------------------------------------
	struct AreaKey
	{
		// Area starting index
		unsigned index;
		// Area count
		unsigned count;
		// Is area reversed
		bool reversed;

		bool operator==( const AreaKey& key ) const
		{
			return key.index == index && key.count == count && key.reversed == reversed;
		}
		operator size_t() const
		{
			return index | ( count << 8 ) | ( reversed ? 0xf0000 : 0 );
		}
	};

	void CreateVertexDeclaration() const;
	void RebuildIndirectBuffers();
	Tr2GpuBufferAL* GetIndirectBuffer( const AreaKey& key );

	// Path to instance geometry resource
	std::string m_instanceGeometryResPath;
	// Instance geometry resource
	ITr2InstanceDataPtr m_instanceGeometryResource;
	// Instance geometry resource loaded from m_instanceGeometryResPath
	ITr2InstanceDataPtr m_loadedGeometryResource;
	// Mesh index of instance data in instance geometry resource
	int m_instanceMeshIndex;
	// Combined vertex declaration
	mutable unsigned int m_vertexDeclaration;

	// Map from area properties to indirect parameter buffers
	std::map<AreaKey, Tr2GpuBufferAL*> m_indirectParams;
	// Exposed instance count buffer pointer
	ITr2GpuBufferPtr m_instanceCount;

	// Min bounds
	Vector3 m_minBounds;
	// Max bounds
	Vector3 m_maxBounds;
};

TYPEDEF_BLUECLASS( Tr2InstancedMesh );
BLUE_DECLARE_VECTOR( Tr2InstancedMesh );

#endif // Tr2InstancedMesh_H
