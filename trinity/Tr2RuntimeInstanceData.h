////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2RuntimeInstanceData_H
#define Tr2RuntimeInstanceData_H

#include "include/ITr2InstanceData.h"
#include "include/ITr2GpuBuffer.h"
#include "Particle/ITr2GenericEmitter.h"
#include "Tr2DeviceResource.h"

BLUE_DECLARE( Tr2ParticleSystem );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2RuntimeInstanceData is an instance data provider for Tr2InstancedMesh that can
//   construct instance data in runtime. The class exposes this functionality to Python
//   so instances can be managed in Python. The class can also be used as an emitter for
//   a particle system. Used in star map.
// See Also:
//   Tr2InstancedMesh
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2RuntimeInstanceData ): 
	public ITr2InstanceData,
	public ITr2GenericEmitter,
	public Tr2DeviceResource,
	public ITr2GpuBuffer
{
public:
	EXPOSE_TO_BLUE();

	Tr2RuntimeInstanceData( IRoot* lockobj = 0 );
	~Tr2RuntimeInstanceData();

	//////////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2InstanceData
	bool IsInstanceDataReady() const override;
	InstanceData GetInstanceData( unsigned int bufferIndex, float screenSize ) const override;
	unsigned int GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const override;
	CcpMath::AxisAlignedBox GetInstanceBufferBoundingBox( unsigned int bufferIndex ) const override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2GpuBuffer
	Tr2BufferAL* GetGpuBuffer( unsigned index );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2GenericEmitter
	void Update( const UpdateArguments& arguments );
	void SpawnParticles( 
		const UpdateArguments& arguments,
		const Vector3* position = nullptr, 
		const Vector3* velocity = nullptr, 
		float rateModifier = 1.0f );
	void SpawnParticles( 
		const UpdateArguments& arguments,
		const Vector3 *positionStart, 
		const Vector3 *positionEnd,
		const Vector3 *velocityStart, 
		const Vector3 *velocityEnd,
		float deltaTime );
	void SetThreadSafeFlag();

	unsigned GetCount() const;
	void SetLayout( const Tr2VertexDefinition& layout );
	const Tr2VertexDefinition& GetLayout() const;
	const void* GetData() const;
	void* GetData( unsigned count );
	void UpdateData();
	void UpdateBoundingBox();
	bool GetBoundingBox( Vector3& minAabb, Vector3& maxAabb ) const;
	void SetBoundingBox( const CcpMath::AxisAlignedBox& aabb );
	void DestroyData();
	unsigned GetStride() const;
	void Spawn();
private:
	void CreateDeclaration();

	std::string m_name;

	// Number of instances
	unsigned m_count;
	// Instance data on CPU
	std::unique_ptr<char> m_data;
	// Per-instance data size
	unsigned m_stride;

	// Vertex declaration handle for instance data
	unsigned m_vertexDeclaration;
	// Vertex layout
	Tr2VertexDefinition m_layout;
	// Instance data as vertex buffer
	Tr2BufferAL m_vb;

	// Bounding box
	CcpMath::AxisAlignedBox m_aabb;

	// System to emit particles to
	Tr2ParticleSystemPtr m_particleSystem;

	bool m_explicitBoundingBox;

	void SaveToGranny( const char* ) const;
};

TYPEDEF_BLUECLASS( Tr2RuntimeInstanceData );

#endif // Tr2RuntimeInstanceData_H