////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PostProcessRenderInfo.h"


Tr2PostProcessRenderInfo::Tr2PostProcessRenderInfo( IRoot* lockobj )
{
	m_black.CreateInstance();
	m_black->SetName( "Black" );
}


Tr2PostProcessRenderInfo::~Tr2PostProcessRenderInfo()
{
	for( auto& texture : m_tempTextures )
	{
		CCP_ASSERT( texture.lockCount == 0 );
		texture.texture->Destroy();
	}
	if( m_black->IsValid() )
	{
		m_black->Destroy();
	}
}


bool Tr2PostProcessRenderInfo::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_sourceBuffer ) )
	{
		SetSourceBuffer( m_sourceBuffer );
	}
	return true;
}

bool Tr2PostProcessRenderInfo::Setup( Tr2RenderContext& renderContext )
{
	if( !m_black->IsValid() )
	{
		m_black->CreateManual( 
			4, // width
			4, // height
			1, // miplevelcount
			Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, // format
			1, // msaaType
			0, // msaaQuality
			Tr2RenderContextEnum::EX_NONE, // flags
			Tr2RenderContextEnum::TEX_TYPE_2D, // texture type
			Tr2CpuUsage::WRITE, // cpu flags
			Tr2GpuUsage::SHADER_RESOURCE // gpu flags
		);

		auto texture = m_black->GetTexture();
		
		// blackify texture
		void* data = nullptr;
		uint32_t pitch = 0;

		if( texture != nullptr && SUCCEEDED( texture->MapForWriting( Tr2TextureSubresource( 0 ), data, pitch, renderContext ) ) )
		{
			uint32_t* mem = (uint32_t*)data;
			memset( mem, 0, 4 * pitch ); // blackify!
			texture->UnmapForWriting( renderContext );
		}
		else
		{
			return false;
		}
	}
	return true;
}

void Tr2PostProcessRenderInfo::SetSourceBuffer( Tr2RenderTarget* sourceBuffer )
{
	m_sourceBuffer = sourceBuffer;
	for( auto& texture : m_tempTextures )
	{
		CCP_ASSERT( texture.lockCount == 0 );
		texture.texture->Destroy();
	}
	m_tempTextures.clear();
}

Tr2PostProcessRenderInfo::Texture Tr2PostProcessRenderInfo::GetSourceBuffer()
{
	return Texture( this, m_sourceBuffer );
}

Tr2PostProcessRenderInfo::Texture Tr2PostProcessRenderInfo::GetBlackTexture()
{
	return Texture( this, m_black );
}

Tr2PostProcessRenderInfo::Texture Tr2PostProcessRenderInfo::GetTempTexture( float sizeFactor, Tr2RenderContextEnum::ExFlag exFlag, Tr2RenderContextEnum::PixelFormat pixelFormat )
{
	CCP_ASSERT( sizeFactor > 0 );
	if( !m_sourceBuffer )
	{
		return Texture();
	}

	if( pixelFormat == Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
	{
		pixelFormat = m_sourceBuffer->GetFormat();
	}

	for( auto& tempTexture : m_tempTextures )
	{
		if( tempTexture.sizeFactor == sizeFactor && tempTexture.exFlag == exFlag && tempTexture.pixelFormat == pixelFormat && tempTexture.lockCount == 0 )
		{
			++tempTexture.lockCount;
			return Texture( this, tempTexture.texture );
		}
	}

	TempTexture tempTexture;
	tempTexture.sizeFactor = sizeFactor;
	tempTexture.exFlag = exFlag;
	tempTexture.lockCount = 1;
	tempTexture.pixelFormat = pixelFormat;
	tempTexture.texture.CreateInstance();
	tempTexture.texture->Create(
		std::max( unsigned( m_sourceBuffer->GetWidth() * sizeFactor ), 1u ),
		std::max( unsigned( m_sourceBuffer->GetHeight() * sizeFactor ), 1u ),
		1,
		pixelFormat,
		1,
		0,
		exFlag );
	m_tempTextures.push_back( tempTexture );
	return Texture( this, tempTexture.texture );
}

void Tr2PostProcessRenderInfo::LockTempTexture( Tr2RenderTarget* texture )
{
	if( !texture || texture == m_sourceBuffer || texture == m_black )
	{
		return;
	}
	for( auto& tempTexture : m_tempTextures )
	{
		if( tempTexture.texture == texture )
		{
			++tempTexture.lockCount;
			return;
		}
	}
	CCP_ASSERT( false );
}

void Tr2PostProcessRenderInfo::ReleaseTempTexture( Tr2RenderTarget* texture )
{
	if( !texture || texture == m_sourceBuffer || texture == m_black )
	{
		return;
	}
	for( auto& tempTexture : m_tempTextures )
	{
		if( tempTexture.texture == texture )
		{
			CCP_ASSERT( tempTexture.lockCount > 0 );
			--tempTexture.lockCount;
			return;
		}
	}
	CCP_ASSERT( false );
}

std::vector<Tr2RenderTargetPtr> Tr2PostProcessRenderInfo::GetAllTempTextures() const
{
	std::vector<Tr2RenderTargetPtr> result;
	result.reserve( m_tempTextures.size() );
	for( auto& tempTexture : m_tempTextures )
	{
		result.push_back( tempTexture.texture );
	}
	return result;
}


Tr2PostProcessRenderInfo::Texture::Texture() :
	m_renderTarget( nullptr ),
	m_renderInfo( nullptr )
{
}

Tr2PostProcessRenderInfo::Texture::Texture( Tr2PostProcessRenderInfo* renderInfo, Tr2RenderTarget* renderTarget ) :
	m_renderTarget( renderTarget ),
	m_renderInfo( renderInfo )
{
}

Tr2PostProcessRenderInfo::Texture::Texture( const Texture& other ) :
	m_renderTarget( other.m_renderTarget ),
	m_renderInfo( other.m_renderInfo )
{
	if( m_renderInfo )
	{
		m_renderInfo->LockTempTexture( m_renderTarget );
	}
}

Tr2PostProcessRenderInfo::Texture& Tr2PostProcessRenderInfo::Texture::operator=( const Texture& other )
{
	if( this == &other )
	{
		return *this;
	}
	if( m_renderInfo )
	{
		m_renderInfo->ReleaseTempTexture( m_renderTarget );
	}
	m_renderInfo = other.m_renderInfo;
	m_renderTarget = other.m_renderTarget;
	if( m_renderInfo )
	{
		m_renderInfo->LockTempTexture( m_renderTarget );
	}
	return *this;
}

Tr2PostProcessRenderInfo::Texture::~Texture()
{
	if( m_renderInfo )
	{
		m_renderInfo->ReleaseTempTexture( m_renderTarget );
	}
}

Tr2RenderTarget* Tr2PostProcessRenderInfo::Texture::GetRenderTarget() const
{
	return m_renderTarget;
}

Tr2PostProcessRenderInfo::Texture::operator Tr2RenderTarget*() const
{
	return m_renderTarget;
}

Tr2RenderTarget* Tr2PostProcessRenderInfo::Texture::operator->() const
{
	return m_renderTarget;
}
