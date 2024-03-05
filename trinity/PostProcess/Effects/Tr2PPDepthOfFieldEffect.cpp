////////////////////////////////////////////////////////////////////////////////
//
// Created:		December 2021
// Copyright:	CCP 2021
//

#include "StdAfx.h"
#include "Tr2PPDepthOfFieldEffect.h"
#include "TriSettingsRegistrar.h"

bool g_postprocessDofEnabled= false;
TRI_REGISTER_SETTING( "postprocessDofEnabled", g_postprocessDofEnabled );

Tr2PPDepthOfFieldEffect::Tr2PPDepthOfFieldEffect( IRoot* lockobj ) :
	m_focalDistance( 0.0f ),
	m_focalLength(0.0f),
	m_scale( 0.0f ),
	m_cocScale( 1.0f ),
	m_foregroundBlurNeeded( true ),
	m_debug( Tr2PPDepthOfFieldEffect::DofDebug_Off ),
	m_bokehShape( Tr2Bokeh::Disk ),
	m_useTAAFriendlyBokeh(true)
{
}

Tr2PPDepthOfFieldEffect::~Tr2PPDepthOfFieldEffect()
{
}

bool Tr2PPDepthOfFieldEffect::IsActive()
{
	return g_postprocessDofEnabled && Tr2PPEffect::IsActive() && m_scale > 0.0f;
}

BlueSharedString Tr2PPDepthOfFieldEffect::GetBokehShapeString() const
{
	switch( m_bokehShape )
	{
	case Tr2Bokeh::Disk:
		return BlueSharedString( "BOKEH_SHAPE_DISK" );
	case Tr2Bokeh::Rectangle:
		return BlueSharedString( "BOKEH_SHAPE_RECTANGLE" );
	case Tr2Bokeh::Triangle:
		return BlueSharedString( "BOKEH_SHAPE_TRIANGLE" );
	default:
		return BlueSharedString( "BOKEH_SHAPE_DISK" );
	}
}
