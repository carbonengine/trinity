// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "TriVariable.h"

#include "include/TriMath.h"
#include "include/ITr2GpuBuffer.h"

#include "Resources/TriTextureRes.h"

#include "Tr2DepthStencil.h"
#include "Tr2RenderTarget.h"
#include "Tr2VariableStore.h"

BLUE_DEFINE_NONEXPOSED( TriVariable );

const Be::ClassInfo* TriVariable::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriVariable, "" )
		MAP_INTERFACE( IRoot )
	EXPOSURE_END()
}

void TriVariable::UseSRV(
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex,
	ResourceFlags flags,
	Tr2RenderContext& renderContext ) const
{
	switch( m_type )
	{
	case TRIVARIABLE_TEXTURE_RES: {
		auto colorSpace = ( flags & RESOURCE_FLAG_SRGB ) != 0 ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
		Tr2TextureAL* tex = nullptr;
		if( m_texture )
		{
			tex = m_texture->GetTexture();
		}
		if( tex )
		{
			renderContext.SetSrv( stage, registerIndex, *tex, colorSpace );
		}
		else
		{
			renderContext.SetSrv( stage, registerIndex, Tr2TextureAL(), colorSpace );
		}
		break;
	}
	case TRIVARIABLE_GPUBUFFER: {
		Tr2BufferAL* buffer = nullptr;
		if( m_gpuBuffer )
		{
			buffer = m_gpuBuffer->GetGpuBuffer( 0 );
		}
		if( buffer )
		{
			renderContext.SetSrv( stage, registerIndex, *buffer );
		}
		else
		{
			renderContext.SetSrv( stage, registerIndex, Tr2BufferAL() );
		}
		break;
	}
	default:
		break;
	}
}

void TriVariable::UseUav(
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex,
	Tr2RenderContext& renderContext ) const
{
	switch( m_type )
	{
	case TRIVARIABLE_TEXTURE_RES: {
		Tr2TextureAL* tex = nullptr;
		if( m_texture )
		{
			tex = m_texture->GetTexture();
		}
		if( tex )
		{
			renderContext.SetUav( stage, registerIndex, *tex );
		}
		else
		{
			renderContext.SetUav( stage, registerIndex, Tr2BufferAL() );
		}
		return;
	}
	case TRIVARIABLE_GPUBUFFER: {
		Tr2BufferAL* buffer = nullptr;
		if( m_gpuBuffer )
		{
			buffer = m_gpuBuffer->GetGpuBuffer( 0 );
		}
		if( buffer )
		{
			renderContext.SetUav( stage, registerIndex, *buffer );
		}
		else
		{
			renderContext.SetUav( stage, registerIndex, Tr2BufferAL() );
		}
		return;
	}
	default:
		break;
	}
	renderContext.SetUav( stage, registerIndex, Tr2BufferAL() );
}

void TriVariable::CopyValueToEffect( Tr2RenderContextEnum::ShaderType inputType,
									 unsigned char* destHandle,
									 size_t size,
									 Tr2RenderContext& renderContext ) const
{
	switch( m_type )
	{
	case TRIVARIABLE_INVALID:
	case TRIVARIABLE_UNKNOWN_FLOAT:
	case TRIVARIABLE_TEXTURE_RES:
	case TRIVARIABLE_GPUBUFFER:
		// Do Nothing
		break;
	case TRIVARIABLE_FLOAT4X4: {
		size_t ts = GetTypeSize();
		// column_major for shaders, pay attention to size of registers
		TriMatrixTranspose(
			reinterpret_cast<Matrix*>( destHandle ),
			reinterpret_cast<const Matrix*>( m_value ),
			(unsigned int)( size < ts ? size : ts ) );
		break;
	}
	default: {
		size_t ts = GetTypeSize();
		memcpy( destHandle, m_value, size < ts ? size : ts );
	}
	}
}

void TriVariable::Invalidate()
{
	Clear();
	m_type = TRIVARIABLE_INVALID;
}

void TriVariable::Clear()
{
	m_texture = nullptr;
	m_gpuBuffer = nullptr;

	memset( m_value, 0, GetTypeSize() );
}
