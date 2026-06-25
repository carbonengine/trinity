// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Range.h"

Range::Range( IRoot* lockobj ) :
	m_isUniform( true ),
	m_centerPoint( 0 ),
	m_minRange( 0 ),
	m_maxRange( 0 ),
	m_sliderRangeMin( 0 ),
	m_sliderRangeMax( 0 )
{
}

Range::~Range()
{
}

void Range::SetCenterPoint( const float var )
{
	float oldCenterDiff = m_centerPoint;
	m_centerPoint = var;
	oldCenterDiff = m_centerPoint - oldCenterDiff;
	m_minRange += oldCenterDiff;
	m_maxRange += oldCenterDiff;
}

void Range::Setup( float rangeCenterPoint, float rangeDeltaFromCenter, float sliderMin, float sliderMax )
{
	m_centerPoint = rangeCenterPoint;
	m_minRange = m_centerPoint - rangeDeltaFromCenter;
	m_maxRange = m_centerPoint + rangeDeltaFromCenter;
	m_sliderRangeMin = sliderMin;
	m_sliderRangeMax = sliderMax;
}

void Range::SetMinRangePoint( const float var )
{
	m_minRange = min( var, m_centerPoint );

	if( m_isUniform )
	{
		m_maxRange = m_centerPoint + ( m_centerPoint - m_minRange );
	}
}

void Range::SetMaxRangePoint( const float var )
{
	m_maxRange = max( var, m_centerPoint );

	if( m_isUniform )
	{
		m_minRange = m_centerPoint - ( m_maxRange - m_centerPoint );
	}
}

float Range::GetCenterPoint() const
{
	return m_centerPoint;
}

float Range::GetMinRangePoint() const
{
	return min( m_minRange, m_sliderRangeMin );
}

float Range::GetMaxRangePoint() const
{
	return min( m_maxRange, m_sliderRangeMax );
}

void Range::ToggleIsUniform()
{
	m_isUniform = !m_isUniform;
	if( m_isUniform )
	{
		FixUniformity();
	}
}

void Range::SetIsUniform( const bool var )
{
	m_isUniform = var;
	if( m_isUniform )
	{
		FixUniformity();
	}
}

void Range::FixUniformity()
{
	float minRangeDelta = m_centerPoint - m_minRange;
	float maxRangeDelta = m_maxRange - m_centerPoint;
	float newDelta = min( minRangeDelta, maxRangeDelta );
	SetMinRangePoint( m_centerPoint - newDelta );
	SetMaxRangePoint( m_centerPoint + newDelta );
}

bool Range::GetIsUniform() const
{
	return m_isUniform;
}

void Range::SetSliderMin( const float var )
{
	m_sliderRangeMin = var;
}

void Range::SetSliderMax( const float var )
{
	m_sliderRangeMax = var;
}

float Range::GetSliderMin() const
{
	return m_sliderRangeMin;
}

float Range::GetSliderMax() const
{
	return m_sliderRangeMax;
}
