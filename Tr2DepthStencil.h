#pragma once
#ifndef Tr2DepthStencil_h_
#define Tr2DepthStencil_h_

BLUE_DECLARE( Tr2DepthStencil );

// -------------------------------------------------------------
// Description:
//   A class to hang on to platform specific pointers needed for
//   a classic depth-and-stencil-buffer setup.
//   This class replaces TriSurface for those use-cases where the
//   surface was just a depth stencil surface.
// -------------------------------------------------------------
BLUE_CLASS( Tr2DepthStencil ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

    Tr2DepthStencil( IRoot* = 0 );
	~Tr2DepthStencil();

	void py__init__(
		Be::OptionalWithDefaultValue<unsigned, 0> width,
		Be::OptionalWithDefaultValue<unsigned, 0> height,
		Be::OptionalWithDefaultValue<Tr2RenderContextEnum::DepthStencilFormat, Tr2RenderContextEnum::DSFMT_UNKNOWN> format,
		Be::OptionalWithDefaultValue<unsigned, 1> msaaType,
		Be::OptionalWithDefaultValue<unsigned, 0> msaaQuality );

	long Create( unsigned width, unsigned height, Tr2RenderContextEnum::DepthStencilFormat format, unsigned msaaType, unsigned msaaQuality );
	long CreateEx( unsigned width, unsigned height, Tr2RenderContextEnum::DepthStencilFormat format, unsigned msaaType, unsigned msaaQuality, unsigned flags );
	bool IsValid() const;
	void Destroy();
	bool IsReadable() const;

	uint32_t GetSharedHandle() const;

	Tr2DepthStencilAL	m_depthStencil;

	operator Tr2DepthStencilAL&() { return m_depthStencil; }	// avoid m_depthStencil->m_depthStencil all over the place
	operator const Tr2DepthStencilAL&() const { return m_depthStencil; }
	
	std::string m_name;
private:
	bool HasALObject( int type, size_t object );
};

TYPEDEF_BLUECLASS( Tr2DepthStencil );

#endif //Tr2DepthStencil_h_
