////////////////////////////////////////////////////////////////////////////////
//
// Created:		11/24/2021
// Copyright:	CCP 2021
//

#pragma once

#include "Tr2PPEffect.h"
namespace Tr2Bokeh
{
	enum Shape
	{
		Disk,
		Rectangle,
		Triangle
	};
}

extern bool g_postprocessDofEnabled;

BLUE_CLASS( Tr2PPDepthOfFieldEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();
		
	enum DofDebug
	{
		DofDebug_Off,
		DofDebug_CoC,
		DofDebug_CoCBlurred,
		DofDebug_BokehBlend,
	};

	Tr2PPDepthOfFieldEffect( IRoot* lockobj = NULL );
	~Tr2PPDepthOfFieldEffect();

	BlueSharedString GetBokehShapeString() const;


	// Tr2PPEffect
	bool IsActive() override;

	float m_focalDistance;
	float m_focalLength;
	float m_scale;
	bool m_foregroundBlurNeeded;
	DofDebug m_debug;
	float m_cocScale;
	Tr2Bokeh::Shape m_bokehShape;
	bool m_useTAAFriendlyBokeh;
};
TYPEDEF_BLUECLASS( Tr2PPDepthOfFieldEffect );
