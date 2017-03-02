#include "StdAfx.h"
#include "Tr2VertexBufferALDx11.h"


#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

using namespace Tr2RenderContextEnum;

// -------------------------------------------------------------
// Description:
//  Create a VB
// Arguments:
// length - number of BYTES to use in the vertex buffer
// -------------------------------------------------------------
ALResult Tr2VertexBufferAL::Create( uint32_t lengthInBytes,
									BufferUsage usage,
									const void* initialData,
									Tr2PrimaryRenderContextAL& renderContext )
{
	return CreateEx( lengthInBytes, usage, initialData, 0, renderContext );
}

ALResult Tr2VertexBufferAL::CreateEx( uint32_t lengthInBytes,
									  BufferUsage usage,
									  const void* initialData,
									  uint32_t/* exFlags */,
									  Tr2PrimaryRenderContextAL& renderContext )
{
	HRESULT hr = Tr2BufferImplAL::Create( lengthInBytes,
									usage,
									D3D11_BIND_VERTEX_BUFFER,
									0,
									initialData,
									renderContext );
	if( SUCCEEDED( hr ) )
	{
		ChangeObjectId();
	}
	return hr;
}

// -------------------------------------------------------------
// Description:
//  Create a VB with reasonable default flags.
// Arguments:
// length - number of BYTES to use in the vertex buffer
// -------------------------------------------------------------
ALResult Tr2VertexBufferAL::Create( uint32_t length,
									Tr2PrimaryRenderContextAL& renderContext )
{
	HRESULT hr = Tr2BufferImplAL::Create( length,
									USAGE_CPU_READ | USAGE_CPU_WRITE,
									D3D11_BIND_VERTEX_BUFFER,
									0,
									nullptr,
									renderContext );
	if( SUCCEEDED( hr ) )
	{
		ChangeObjectId();
	}
	return hr;
}

Tr2VertexBufferAL& Tr2VertexBufferAL::operator=( Tr2VertexBufferAL&& other )
{
	Tr2BufferImplAL::operator=( std::move( other ) );
	ChangeObjectId();
	return *this;
}

#endif
