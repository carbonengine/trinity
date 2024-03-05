#include "StdAfx.h"
#include "Tr2TextureAtlasMan.h"
#include "Tr2TextureAtlas.h"

using namespace Tr2RenderContextEnum;

CTr2TextureAtlasMan s_textureAtlasManInstance;
Tr2TextureAtlasMan* g_textureAtlasMan = &s_textureAtlasManInstance;

BLUE_REGISTER_GLOBAL_AS_MODULE_OBJECT( "textureAtlasMan", g_textureAtlasMan );

Tr2TextureAtlasMan::Tr2TextureAtlasMan( IRoot* lockobj ) :
	PARENTLOCK( m_atlases ),
	m_atlasesMutex( "atlasMan", "m_atlasesMutex" )
{
}

Tr2TextureAtlas* Tr2TextureAtlasMan::FindAtlas( Tr2RenderContextEnum::PixelFormat fmt )
{
	CcpAutoMutex lock( m_atlasesMutex );

	for( auto it = m_atlases.begin(); it != m_atlases.end(); ++it )
	{
		if( (*it)->GetFormat() == fmt )
		{
			return *it;
		}
	}

	return NULL;
}

// TODO: Error handling
void Tr2TextureAtlasMan::AddAtlas( Tr2RenderContextEnum::PixelFormat fmt, unsigned int width, unsigned int height )
{
	CCP_ASSERT( !FindAtlas( fmt ) );

	CcpAutoMutex lock( m_atlasesMutex );

	Tr2TextureAtlasPtr atlas;

	atlas.CreateInstance();
	atlas->Initialize( fmt, width, height );
	m_atlases.Insert( -1, atlas );
}

void Tr2TextureAtlasMan::AddAtlasWrap( unsigned int fmt, unsigned int width, unsigned int height )
{
	AddAtlas( (Tr2RenderContextEnum::PixelFormat)fmt, width, height );
}

// TODO: Error handling
void Tr2TextureAtlasMan::RemoveAtlas( Tr2RenderContextEnum::PixelFormat fmt )
{
	CcpAutoMutex lock( m_atlasesMutex );

	ssize_t ix = 0;
	for( auto it = m_atlases.begin(); it != m_atlases.end(); ++it, ++ix )
	{
		if( (*it)->GetFormat() == fmt )
		{
			m_atlases.Remove( ix );
			break;
		}
	}
}

void Tr2TextureAtlasMan::RemoveAtlasWrap( unsigned fmt )
{
	RemoveAtlas( (Tr2RenderContextEnum::PixelFormat)fmt );
}
