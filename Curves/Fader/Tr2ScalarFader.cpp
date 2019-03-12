////////////////////////////////////////////////////////////
//
//    Created:   November 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "Tr2ScalarFader.h"

#include "Eve/EveUpdateContext.h"
#include "include/TriMath.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
Tr2ScalarFader::Tr2ScalarFader( IRoot* lockobj ) :
	m_value( 0.f ),
	m_fading( 0.f ),
	m_fadeTime( -1.f ),
	m_kickInLength( 3.f )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Byebye
// --------------------------------------------------------------------------------
Tr2ScalarFader::~Tr2ScalarFader()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Update the fading progress if there is any going on
// --------------------------------------------------------------------------------
void Tr2ScalarFader::Update( EveUpdateContext& updateContext )
{
	if( m_fading != 0.f )
	{
		m_value += m_fading * updateContext.GetDeltaT();
		if( m_value < 0.f )
		{
			m_value = 0.f;
			m_fading = 0.f;
		}
		else if( m_value > 1.f )
		{
			m_value = 1.f;
			m_fading = 0.f;
		}
	}

	if( m_fadeTime >= 0.f )
	{
		m_fadeTime += updateContext.GetDeltaT();
		if( m_fadeTime > m_kickInLength )
		{
			m_fadeTime = -1.f;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Start a fadein or fadeout
// --------------------------------------------------------------------------------
void Tr2ScalarFader::StartFade( bool isFadeIn, float fadeLength )
{
	m_kickInLength = fadeLength;
	m_fading = isFadeIn ? 1.f / m_kickInLength : -1.f / m_kickInLength;
	if( isFadeIn )
	{
		m_fadeTime = 0.f;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Get the fade value
// --------------------------------------------------------------------------------
float Tr2ScalarFader::GetFaderValue() const
{
	return m_value;
}

// --------------------------------------------------------------------------------
// Description:
//   Get a kickin value in case a fedin process is going on
// --------------------------------------------------------------------------------
float Tr2ScalarFader::GetKickInValue() const
{
	// is only during fadetime
	if( m_fadeTime < 0.f )
	{
		return 0.f;
	}
	// calc some simple kicking curve
	float x = TriClamp( m_fadeTime / m_kickInLength, 0.f, 1.f );
	return powf( sinf( TRI_PI * powf( x, 0.66f ) ), 3.f );
}

// --------------------------------------------------------------------------------
// Description:
//   Small access helper to tell if we have a non zero value on this fader
// --------------------------------------------------------------------------------
bool Tr2ScalarFader::IsZero() const
{
	return ( m_value == 0.f ) && ( m_fading == 0.f );
}

// --------------------------------------------------------------------------------
// Description:
//   Small access helper to tell if we have a non zero kick in value
// --------------------------------------------------------------------------------
bool Tr2ScalarFader::IsKickInZero() const
{
	return ( m_fadeTime <= 0.f );
}



