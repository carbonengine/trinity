// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2RuntimeGpuBuffer.h"

Tr2BufferAL* Tr2RuntimeGpuBuffer::GetGpuBuffer( unsigned )
{
	return &m_buffer;
}

void Tr2RuntimeGpuBuffer::SetGpuBuffer( const Tr2BufferAL& buffer )
{
	m_buffer = buffer;
}
