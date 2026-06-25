// Copyright © 2013 CCP ehf.

#pragma once
#ifndef Tr2GpuBuffer_H
#define Tr2GpuBuffer_H

#include "include/ITr2GpuBuffer.h"

// --------------------------------------------------------------------------------------
// Description:
//   A Blue-exposed wrapper around Tr2GpuBufferAL class for typed buffers, i.e. buffers
//   that have a valid pixel format. The creation parameters for the class are persisted,
//   but the contents of the buffer is not.
// See Also:
//   Tr2BufferAL
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2GpuBuffer ) :
	public ITr2GpuBuffer,
	public INotify,
	public IInitialize,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	enum CreationFlag
	{
		// Can the buffer be locked with write-only access
		CPU_WRITABLE = 1,
		// Is the buffer used for GPU write access
		GPU_WRITABLE = 2,
		// Is the buffer used for indirect draw calls
		DRAW_INDIRECT = 4,
	};

	typedef uint32_t CreationFlags;

	Tr2GpuBuffer( IRoot* = 0 );
	~Tr2GpuBuffer();

	bool Initialize();

	bool OnModified( Be::Var * value );

	Tr2BufferAL* GetGpuBuffer( unsigned index );

	ALResult Create( uint32_t count, Tr2RenderContextEnum::PixelFormat format, CreationFlags m_creationFlags );
	bool IsValid() const;
	uint32_t GetCount() const;

	void SetName( const char* name );

	Tr2RenderContextEnum::PixelFormat GetFormat() const
	{
		return m_format;
	}

	operator Tr2BufferAL&()
	{
		return m_buffer;
	}
	operator const Tr2BufferAL&() const
	{
		return m_buffer;
	}

	virtual void ReleaseResources( TriStorage s );

private:
	virtual bool OnPrepareResources();
	ALResult __init__( Be::Optional<uint32_t> count, Be::Optional<Tr2RenderContextEnum::PixelFormat> format, CreationFlags m_creationFlags );

	ALResult CreateBuffer();

	// AL buffer
	Tr2BufferAL m_buffer;

	// Number of elements in the buffer
	uint32_t m_count;
	// Type of elements
	Tr2RenderContextEnum::PixelFormat m_format;
	CreationFlags m_creationFlags;

	std::string m_name;
};

TYPEDEF_BLUECLASS( Tr2GpuBuffer );

#endif