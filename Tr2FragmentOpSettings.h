#pragma once
#ifndef Tr2FragmentOpSettings_h_
#define Tr2FragmentOpSettings_h_


#include "Tr2RenderContextEnum.h"

// -------------------------------------------------------------
// Description:
//	Keep track of states that are no longer natively supported in DX11/OpenGLES2.0, so we can emulate them
//	in shader code using a constant buffer to drive their settings.
// -------------------------------------------------------------
struct Tr2FragmentOpSettings
{
	// nested struct to keep track of what's ideally set, before we transform the values for
	// shader optimizations.
	struct TAlphaTestParameters
	{
		uint32_t							m_alphaTestEnabled;
		int32_t								m_alphaTestRef;
		Tr2RenderContextEnum::CompareFunc	m_alphaTestFunc;
	};


	// Maps 1:1 with the constant buffer, so use 32 bit values only
	int32_t								m_invertedAlphaTest;
	int32_t								m_alphaTestRef;
	int32_t								m_alphaTestFunc;
		
	uint32_t							m_clipPlaneEnable;

	enum { MAX_CLIP_PLANES = 4 };

	float								m_clipPlane[MAX_CLIP_PLANES][4];

	float								m_renderTargetSize[4];

	uint32_t							m_numLights;
	uint32_t							filler[3];

	enum
	{
		DIRTY_BLEND			= 1 << 0,
		DIRTY_DEPTHSTENCIL	= 1 << 1,
		DIRTY_RASTERIZER	= 1 << 2,
		DIRTY_FRAGMENTOP	= 1 << 3,
		DIRTY_PATCH_PS		= 1 << 4,
		DIRTY_PATCH_VS		= 1 << 5,

		HANDLED_BUT_NO_CHANGES	= 0xFFffFFffu
	};

	// returns zero if not a state we care about, else returns a flag of what's dirty
	uint32_t SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value, TAlphaTestParameters& alphaTestParameters );

	uint32_t SetClipPlane( uint32_t planeIndex, const float* planeEq );

	uint32_t SetNumberOfLights( uint32_t numLights );

	void	UpdateContents( const TAlphaTestParameters& alphaTestParameters );
};
	
#endif //Tr2FragmentOpSettings_h_
