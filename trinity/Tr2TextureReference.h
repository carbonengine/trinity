// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2TextureReference_H
#define Tr2TextureReference_H

#include "ITr2TextureProvider.h"


// -------------------------------------------------------------
// Description:
//   A simple blue-exposed class that stores a Tr2TextureAL. It
//   can be used to controll texture lifetime with reference
//   counting. It also allows using internal engine textures
//   with variable stores.
// -------------------------------------------------------------
BLUE_CLASS( Tr2TextureReference ) :
	public ITr2TextureProvider
{
public:
	EXPOSE_TO_BLUE();

	Tr2TextureReference( IRoot* lockobj = nullptr );

	virtual Tr2TextureAL* GetTexture();
	OnTextureChangeEvent& OnTextureChange() override;

	BeResultChoice<BlueStdResult, ImageIO::Result> Save( const std::wstring& path );

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetDepth() const;
	uint32_t GetType() const;
	uint32_t GetMipCount() const;
	uint32_t GetArraySize() const;
	Tr2RenderContextEnum::PixelFormat GetFormat() const;
	std::string GetName() const;

private:
	Tr2TextureAL m_texture;
	OnTextureChangeEvent m_onTextureChange;
};

TYPEDEF_BLUECLASS( Tr2TextureReference );


// -------------------------------------------------------------
// Description:
//   Similar to Tr2TextureReference, but stores a pointer to
//   Tr2TextureAL. The user of this object needs to store
//   Tr2TextureAL object and manage lifetime.
// -------------------------------------------------------------
BLUE_CLASS( Tr2TransientTextureReference ) :
	public ITr2TextureProvider
{
public:
	EXPOSE_TO_BLUE();

	Tr2TransientTextureReference( IRoot* lockobj = nullptr );

	virtual Tr2TextureAL* GetTexture();
	OnTextureChangeEvent& OnTextureChange() override;

	void SetTexture( Tr2TextureAL * texture );

private:
	Tr2TextureAL* m_texture;
	OnTextureChangeEvent m_onTextureChange;
};

TYPEDEF_BLUECLASS( Tr2TransientTextureReference );

#endif