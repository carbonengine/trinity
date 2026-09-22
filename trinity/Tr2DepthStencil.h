// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2DepthStencil_h_
#define Tr2DepthStencil_h_

#include "ITr2TextureProvider.h"
#include "Tr2DeviceResource.h"

BLUE_DECLARE( Tr2DepthStencil );

// -------------------------------------------------------------
// Description:
//   A class to hang on to platform specific pointers needed for
//   a classic depth-and-stencil-buffer setup.
//   This class replaces TriSurface for those use-cases where the
//   surface was just a depth stencil surface.
// -------------------------------------------------------------
BLUE_CLASS( Tr2DepthStencil ) :
	public ITr2TextureProvider,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	Tr2DepthStencil( IRoot* = 0 );
	~Tr2DepthStencil();

	virtual Tr2TextureAL* GetTexture();
	OnTextureChangeEvent& OnTextureChange() override;

	void py__init__(
		Be::OptionalWithDefaultValue<unsigned, 0> width,
		Be::OptionalWithDefaultValue<unsigned, 0> height,
		Be::OptionalWithDefaultValue<Tr2RenderContextEnum::DepthStencilFormat, Tr2RenderContextEnum::DSFMT_UNKNOWN> format,
		Be::OptionalWithDefaultValue<unsigned, 1> msaaType,
		Be::OptionalWithDefaultValue<unsigned, 0> msaaQuality,
		Be::OptionalWithDefaultValue<Tr2RenderContextEnum::ExFlag, Tr2RenderContextEnum::EX_NONE> flags );

	long Create(
		unsigned width,
		unsigned height,
		Tr2RenderContextEnum::DepthStencilFormat format,
		unsigned msaaType,
		unsigned msaaQuality,
		Tr2RenderContextEnum::ExFlag flags = Tr2RenderContextEnum::EX_NONE );

	bool IsValid() const;
	void Destroy();
	bool IsReadable() const;

	uintptr_t GetSharedHandle() const;

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetMsaaSamples() const;
	uint32_t GetMsaaQuality() const;
	uint32_t GetMipCount() const;
	Tr2RenderContextEnum::DepthStencilFormat GetFormat() const;

	Tr2TextureAL m_depthStencil;

	operator Tr2TextureAL&()
	{
		return m_depthStencil;
	} // avoid m_depthStencil->m_depthStencil all over the place
	operator const Tr2TextureAL&() const
	{
		return m_depthStencil;
	}

	void SetName( const char* name );
	std::string GetName() const;

protected:
	virtual void ReleaseResources( TriStorage s );
	virtual bool OnPrepareResources();

private:
	bool HasALObject( int type, size_t object );

	uint32_t m_width;
	uint32_t m_height;
	Tr2RenderContextEnum::DepthStencilFormat m_format;
	Tr2MsaaDesc m_msaa;
	Tr2RenderContextEnum::ExFlag m_flags;
	std::string m_name;
	OnTextureChangeEvent m_onTextureChange;
};

TYPEDEF_BLUECLASS( Tr2DepthStencil );

#endif //Tr2DepthStencil_h_
