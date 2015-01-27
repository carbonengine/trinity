#include "StdAfx.h"

#include "Tr2InteriorFlareData.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorFlareData default constructor
// --------------------------------------------------------------------------------------
Tr2InteriorFlareData::Tr2InteriorFlareData( IRoot* lockobj )
:	m_positionWeight( 1.f, 1.f ),
	m_size( 0.1f, 0.1f ),
	m_textureOffset( 0.f, 0.f ),
	m_textureSize( 1.f, 1.f ),
	m_rotation( false ),
	m_directionalStretch( 1.0f, 1.0f ),
	m_edgeFadeDistance( 1.f ),
	m_centerFadeMinRadius( 0.0f ),
	m_centerFadeMaxRadius( 0.0f )
{
}
