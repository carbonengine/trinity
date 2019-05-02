#include "StdAfx.h"
#include "Tr2FragmentOpSettings.h"
#include "ALLog.h"

static_assert( sizeof( Tr2FragmentOpSettings ) == 16 + 4*20 + 16, "fragment op settings don't fit the constant buffer" );

using namespace Tr2RenderContextEnum;

uint32_t Tr2FragmentOpSettings::SetRenderState( RenderState state, uint32_t value, TAlphaTestParameters& atp )
{
	switch( state )
	{
		// ---------------------------------- DX9 emulation
	case RS_ALPHATESTENABLE:
		if( ( atp.m_alphaTestEnabled != 0 ) != ( value != 0 ) )
		{
			atp.m_alphaTestEnabled = value ? 1 : 0;
			return DIRTY_FRAGMENTOP | DIRTY_PATCH_PS;
		}
		return (uint32_t)HANDLED_BUT_NO_CHANGES;

	case RS_ALPHAREF:
		if( atp.m_alphaTestRef != (int32_t)value )
		{
			atp.m_alphaTestRef = value;
			return DIRTY_FRAGMENTOP;
		}
		return (uint32_t)HANDLED_BUT_NO_CHANGES;

	case RS_ALPHAFUNC:
		if( (uint32_t)atp.m_alphaTestFunc != value )
		{
			uint32_t dirty = ( atp.m_alphaTestEnabled && 
								( atp.m_alphaTestFunc != CMP_ALWAYS ) == ( value == CMP_ALWAYS ) )	? DIRTY_PATCH_PS : 0;

			atp.m_alphaTestFunc = static_cast<CompareFunc>( value );
			return dirty | DIRTY_FRAGMENTOP;
		}
		return (uint32_t)HANDLED_BUT_NO_CHANGES;

	case RS_CLIPPLANEENABLE:
		if( m_clipPlaneEnable != value )
		{
			m_clipPlaneEnable = value;
			return DIRTY_FRAGMENTOP | DIRTY_PATCH_VS;			
		}
		return (uint32_t)HANDLED_BUT_NO_CHANGES;

	default:
		return 0;
	}
}

uint32_t Tr2FragmentOpSettings::SetNumberOfLights( uint32_t numLights )
{
	if( numLights == m_numLights )
	{
		return 0;
	}
	m_numLights = numLights;
	return DIRTY_FRAGMENTOP;
}


void Tr2FragmentOpSettings::UpdateContents( const TAlphaTestParameters& alphaTestParameters )
{
	if( !alphaTestParameters.m_alphaTestEnabled )
	{
		return;
	}

	switch( alphaTestParameters.m_alphaTestFunc )
	{
	case CMP_NEVER:
		m_alphaTestFunc = 0;
		m_invertedAlphaTest = 1;
		m_alphaTestRef = -256;
		break;
	case CMP_LESS:
		m_alphaTestFunc = 0;
		m_invertedAlphaTest = -1;
		m_alphaTestRef = alphaTestParameters.m_alphaTestRef - 1;
		break;
	case CMP_EQUAL:
		m_alphaTestFunc = 1;
		m_invertedAlphaTest = 0;
		m_alphaTestRef = alphaTestParameters.m_alphaTestRef;
		break;
	case CMP_LESSEQUAL:
		m_alphaTestFunc = 0;
		m_invertedAlphaTest = -1;
		m_alphaTestRef = alphaTestParameters.m_alphaTestRef;
		break;
	case CMP_GREATER:
		m_alphaTestFunc = 0;
		m_invertedAlphaTest = 1;
		m_alphaTestRef = -alphaTestParameters.m_alphaTestRef - 1;
		break;
	case CMP_NOTEQUAL:
		m_alphaTestFunc = 1;
		m_invertedAlphaTest = 1;
		m_alphaTestRef = alphaTestParameters.m_alphaTestRef;
		break;
	case CMP_GREATEREQUAL:
		m_alphaTestFunc = 0;
		m_invertedAlphaTest = 1;
		m_alphaTestRef = -alphaTestParameters.m_alphaTestRef;
		break;
	default:
		m_alphaTestFunc = 0;
		m_invertedAlphaTest = 0;
		m_alphaTestRef = 1;
		break;
	}
}
