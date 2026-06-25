// Copyright © 2026 CCP ehf.

#pragma once
#ifndef Tr2DirectInstanceData_H
#define Tr2DirectInstanceData_H

#include "include/ITr2InstanceData.h"
#include "include/ITr2GpuBuffer.h"
#include "Tr2DeviceResource.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2DirectInstanceData is an instance data provider. Unlike Tr2RuntimeInstanceData,
//   it does not maintain a CPU copy and uses direct GPU buffer access for write-often
//   scenarios.
// See Also:
//   Tr2RuntimeInstanceData
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2DirectInstanceData ) :
	public ITr2InstanceData,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();
	Tr2DirectInstanceData( IRoot* lockobj = 0 );
	void ReleaseResources( TriStorage s );

private:
	bool OnPrepareResources();

public:
	// ITr2InstanceData
	bool IsInstanceDataReady() const override;
	InstanceData GetInstanceData( unsigned int bufferIndex, float screenSize ) const override;
	unsigned int GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const override;
	CcpMath::AxisAlignedBox GetInstanceBufferBoundingBox( unsigned int bufferIndex ) const override;
	// Tr2DirectInstanceData
	unsigned GetCount() const;
	void SetLayout( const Tr2VertexDefinition& layout );
	const Tr2VertexDefinition& GetLayout() const;
	void* GetData( unsigned count );
	void UpdateData();
	void SetBoundingBox( const CcpMath::AxisAlignedBox& aabb );
	void DestroyData();
	unsigned GetStride() const;

private:
	unsigned m_count;
	unsigned m_stride;
	unsigned m_vertexDeclaration;
	Tr2VertexDefinition m_layout;
	Tr2BufferAL m_vertexBuffer;

	CcpMath::AxisAlignedBox m_aabb;
};

TYPEDEF_BLUECLASS( Tr2DirectInstanceData );

#endif // Tr2DirectInstanceData_H
