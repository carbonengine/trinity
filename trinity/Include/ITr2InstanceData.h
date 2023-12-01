////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef ITr2InstanceData_H
#define ITr2InstanceData_H

class Tr2RenderContext;

BLUE_DECLARE_INTERFACE( ITr2InstanceData );

// --------------------------------------------------------------------------------------
// Description:
//   ITr2InstanceData is an interface for instance data provider that is intended to 
//   work with Tr2InstancedMesh class. Classes implementing ITr2InstanceData interface
//   are able to provide vertex buffers and declarations to be used as instance stream
//   for instanced rendering.
// See Also:
//   Tr2InstancedMesh, TriGeometryRes, Tr2SpriteParticleSystem
// --------------------------------------------------------------------------------------
BLUE_INTERFACE( ITr2InstanceData ) : public IRoot
{
	// --------------------------------------------------------------------------------------
	// Description:
	//   Query if the instance data is ready to be used. For example, for TriGeometryRes this
	//   is equivalent to checking if the resource is good.
	// Return Value:
	//   true if data is ready to be used
	//   false otherwise
	// --------------------------------------------------------------------------------------
	virtual bool IsInstanceDataReady() const = 0;

	struct InstanceData
	{
		Tr2BufferAL buffer;
		uint32_t stride;
		uint32_t count;
	};

	virtual InstanceData GetInstanceData( unsigned int bufferIndex, float screenSize ) const = 0;

	// --------------------------------------------------------------------------------------
	// Description:
	//   Returns returns vertex declaration handle for the given instance buffer index.
	// Arguments:
	//   bufferIndex - instance buffer index
	// Return Value:
	//   vertex declaration handle
	// --------------------------------------------------------------------------------------
	virtual unsigned int GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const = 0;

	virtual CcpMath::AxisAlignedBox GetInstanceBufferBoundingBox( unsigned int bufferIndex ) const = 0;
};

#endif // ITr2InstanceData_H