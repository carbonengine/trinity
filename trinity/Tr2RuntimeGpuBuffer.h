// Copyright © 2026 CCP ehf.

#pragma once

#include "Include/ITr2GpuBuffer.h"


BLUE_CLASS( Tr2RuntimeGpuBuffer ) :
	public ITr2GpuBuffer
{
public:
	EXPOSE_TO_BLUE();

	Tr2BufferAL* GetGpuBuffer( unsigned index ) override;
	void SetGpuBuffer( const Tr2BufferAL& buffer );

	Tr2BufferAL m_buffer;
};

TYPEDEF_BLUECLASS( Tr2RuntimeGpuBuffer );
