//////////////////////////////////////////////////////////////////////////
//
// Copyright CCP 2010
//
#pragma once

#ifndef Tr2TextureAtlasMan_h
#define Tr2TextureAtlasMan_h



BLUE_DECLARE( Tr2TextureAtlasMan );
BLUE_DECLARE( Tr2TextureAtlas );
BLUE_DECLARE_VECTOR_RO( Tr2TextureAtlas );

extern Tr2TextureAtlasMan* g_textureAtlasMan;

// A Tr2TextureAtlasMan holds Tr2TextureAtlas objects. It acts as
// centralized depository for finding an atlas for a given texture
// format.
class Tr2TextureAtlasMan:
     public IRoot
{
public:
    EXPOSE_TO_BLUE();
    Tr2TextureAtlasMan( IRoot* lockobj = NULL );

	void AddAtlas( Tr2RenderContextEnum::PixelFormat fmt, unsigned int width, unsigned int height );
	void RemoveAtlas( Tr2RenderContextEnum::PixelFormat fmt );
	Tr2TextureAtlas* FindAtlas( Tr2RenderContextEnum::PixelFormat fmt );

private:
	void AddAtlasWrap( unsigned fmt, unsigned int width, unsigned int height );
	void RemoveAtlasWrap( unsigned fmt );

private:
	PTr2TextureAtlasVectorRO m_atlases;
	
	// FindAtlas may be called on a background thread. We need to protect
	// access to the m_atlases vector in case an atlas is added as
	// we're looking for an atlas.
	CcpMutex m_atlasesMutex;
};

TYPEDEF_BLUECLASS( Tr2TextureAtlasMan );

#endif //Tr2TextureAtlasMan_h
