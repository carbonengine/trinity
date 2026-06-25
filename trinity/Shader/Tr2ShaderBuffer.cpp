// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2ShaderBuffer.h"
#include "Tr2Renderer.h"
#include "Tr2RenderContext.h"

Tr2ShaderBuffer::Tr2ShaderBuffer( IRoot* lockobj ) :
	m_data( nullptr ),
	m_size( 0 ),
	m_shaderType( Tr2RenderContextEnum::PIXEL_SHADER )
{
}

Tr2ShaderBuffer::~Tr2ShaderBuffer()
{
	if( m_data )
	{
		CCP_FREE( m_data );
		m_data = nullptr;
		m_size = 0;
	}
}

void Tr2ShaderBuffer::ApplyBuffer( Tr2RenderContext& renderContext )
{
	FillAndSetConstants( *renderContext.GetConstantBuffer( m_shaderType ), m_data, m_size, m_shaderType, Tr2Renderer::GetPerObjectStartRegister( m_shaderType ), renderContext );
}

void Tr2ShaderBuffer::SetData( void* data, int size )
{
	if( m_data && m_size != size )
	{
		CCP_FREE( m_data );
		m_data = nullptr;
		m_size = 0;
	}
	if( !size )
	{
		return;
	}
	if( !m_data )
	{
		m_data = CCP_MALLOC( "Tr2ShaderBuffer", size );
		m_size = size;
	}
	memcpy( m_data, data, size );
}

void Tr2ShaderBuffer::SetDataFromScript( const char* data, int size )
{
	SetData( (void*)data, size );
}