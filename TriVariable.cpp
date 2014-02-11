#include "StdAfx.h"

#include "TriVariable.h"

#include "include/TriMath.h"

#include "Resources/TriTextureRes.h"

#include "Tr2DepthStencil.h"
#include "Tr2RenderTarget.h"
#include "Tr2VariableStore.h"
#include "blue/include/TransGaming.h"

BLUE_DEFINE_NONEXPOSED( TriVariable );

const Be::ClassInfo* TriVariable::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriVariable, "" )
		MAP_INTERFACE( IRoot )
	EXPOSURE_END()
}


void TriVariable::GetValueTextureAL( Tr2TextureAL*& value ) const
{ 
	CCP_ASSERT( m_type == TRIVARIABLE_TEXTURE_AL );
	value = nullptr;
	if( m_depthStencil )
	{
		if( m_depthStencil->m_depthStencil.GetTexture().IsValid() )
		{
			value = &m_depthStencil->m_depthStencil.GetTexture();
		}
	}
	else if( m_renderTarget )
	{
		if( m_renderTarget->GetRenderTarget().GetTexture().IsValid() )
		{
			value = &m_renderTarget->GetRenderTarget().GetTexture();
		}
	}
	else if( Tr2TextureAL* tex = *(Tr2TextureAL**)m_value )
	{
		value = *(Tr2TextureAL**)m_value;
	}
}

void TriVariable::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
										unsigned char* destHandle, 
										size_t size,
										Tr2RenderContext &renderContext ) const
{
	if( m_multithreaded )
	{
		TriVariable* lookup = renderContext.GetVariableStore().FindLocalVariable( m_name.c_str() );
		if( lookup && lookup != this )
		{
			lookup->CopyValueToEffect( inputType, destHandle, size, renderContext );
			return;
		}
	}
	switch( m_type )
	{
	case TRIVARIABLE_INVALID:
	case TRIVARIABLE_UNKNOWN_FLOAT:
	case TRIVARIABLE_UNKNOWN_TEXTURE:
	case TRIVARIABLE_IROOT:
		// Do Nothing
		break;
	case TRIVARIABLE_FLOAT4X4:
		{
			size_t ts = GetTypeSize();
			// column_major for shaders, pay attention to size of registers
			TriMatrixTranspose( (Matrix*)destHandle, (Matrix*)m_value, (unsigned int)(size < ts ? size : ts) );
			break;
		}
	case TRIVARIABLE_TEXTURE_AL:
		{
			bool isSet = false;
			uint32_t samplerIx = *destHandle;
			if( m_depthStencil )
			{
				if( m_depthStencil->m_depthStencil.GetTexture().IsValid() )
				{
					auto colorSpace = ( size & RESOURCE_FLAG_SRGB ) != 0 ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
					renderContext.m_esm.ApplyTexture( inputType, samplerIx, m_depthStencil->m_depthStencil.GetTexture(), colorSpace );					
					isSet = true;
				}
			}
			else if( m_renderTarget )
			{
				if( m_renderTarget->GetRenderTarget().GetTexture().IsValid() )
				{
					auto colorSpace = ( size & RESOURCE_FLAG_SRGB ) != 0 ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
					renderContext.m_esm.ApplyTexture( inputType, samplerIx, m_renderTarget->GetRenderTarget().GetTexture(), colorSpace );					
					isSet = true;
				}
			}
			else if( Tr2TextureAL* tex = *(Tr2TextureAL**)m_value )
			{
				auto colorSpace = ( size & RESOURCE_FLAG_SRGB ) != 0 ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
				renderContext.m_esm.ApplyTexture( inputType, samplerIx, *tex, colorSpace );
				isSet = true;
			}

			if( !isSet && !IsTransgaming() )
			{
				// For some reason doing this on the Mac seems to cause more problems - we get a black
				// screen on character select, at least on some older Macs with 9400m or 320m video cards
				renderContext.m_esm.ApplyTexture( inputType, samplerIx, nullTX );
			}
			break;
		}
	case TRIVARIABLE_TEXTURE_RES:
		{
			if( m_texture )
			{
				uint32_t samplerIx = *destHandle;
				auto colorSpace = ( size & RESOURCE_FLAG_SRGB ) != 0 ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
				if( Tr2TextureAL* tex = m_texture->GetTexture() )
				{		
					renderContext.m_esm.ApplyTexture( inputType, samplerIx, *tex, colorSpace );
				}
				else
				{
					renderContext.m_esm.ApplyTexture( inputType, samplerIx, nullTX, colorSpace );
				}
			}
			break;
		}
	default:
		{
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
	m_texture		= nullptr;	
	m_depthStencil	= nullptr;
	m_renderTarget	= nullptr;

	memset( m_value, 0, GetTypeSize() );
}
