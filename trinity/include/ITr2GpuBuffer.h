// Copyright © 2013 CCP ehf.

#pragma once
#ifndef ITr2GpuBuffer_H
#define ITr2GpuBuffer_H

// --------------------------------------------------------------------------------------
// Description:
//   An interface for Blue-exposed GPU buffers that wrap Tr2GpuBufferAL objects.
// See Also:
//   Tr2GpuBuffer, TriGeometryRes, Tr2ParticleSystem
// --------------------------------------------------------------------------------------
BLUE_INTERFACE( ITr2GpuBuffer ) :
	public IRoot
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   Returns an AL buffer object. If the implementation supports multiple buffers
	//   (like TriGeometryRes), the index parameter tells which buffer to return.
	// Arguments:
	//   index - Buffer index
	// Return Value:
	//   Pointer to AL buffer or NULL
	// ----------------------------------------------------------------------------------
	virtual Tr2BufferAL* GetGpuBuffer( unsigned index ) = 0;
};

#endif