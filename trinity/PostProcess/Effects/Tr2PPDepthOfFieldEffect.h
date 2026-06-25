// Copyright © 2021 CCP ehf.

#pragma once

#include "Tr2PPEffect.h"
namespace Tr2Bokeh
{
enum Shape
{
	Disk,
	Triangle,
	Rectangle,
	Pentagon,
	Hexagon,
	Heart
};

extern const Be::VarChooser BokehShapeChooser[];
}

extern bool g_postprocessDofEnabled;

BLUE_CLASS( Tr2PPDepthOfFieldEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPDepthOfFieldEffect( IRoot* lockobj = NULL );
	~Tr2PPDepthOfFieldEffect();

	BlueSharedString GetBokehShapeString() const;


	// Tr2PPEffect
	bool IsActive() override;

	float m_focalDistance;
	float m_focalLength;
	float m_scale;
	bool m_foregroundBlurNeeded;
	float m_cocScale;
	Tr2Bokeh::Shape m_bokehShape;
	bool m_useTAAFriendlyBokeh;
};
TYPEDEF_BLUECLASS( Tr2PPDepthOfFieldEffect );
