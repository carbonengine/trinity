#include "StdAfx.h"
#include "TriFloatParameter.h"
#include "ITr2ShaderState.h"

TriFloatParameter::TriFloatParameter(IRoot* lockobj):
	m_value( 1.0f ),
	m_isUsedByEffect( false )
{
}

TriFloatParameter::~TriFloatParameter()
{
}

bool TriFloatParameter::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_name ) )
	{
		RebuildEffectHandles( m_cachedEffect );
	}
	return true;
}

const char* TriFloatParameter::GetParameterName() const
{
	return m_name.c_str();
}

// --------------------------------------------------------------------------------------
// Description:
//   Determines whether the value of this float parameter is 0 and be ignored when
//   building the material situation.
// Return Value:
//   true, if m_value is within epsilon of 0
//   false, otherwise
// --------------------------------------------------------------------------------------
bool TriFloatParameter::IsZeroOrNull( void ) const
{
	static const float epsilon = 0.0000001f;

	return fabsf( m_value ) < epsilon;
}

void TriFloatParameter::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
											unsigned char* destHandle, 
											size_t size,
											Tr2RenderContext &renderContext ) const
{
	memcpy( destHandle, &m_value, size < sizeof(m_value) ? size : sizeof( m_value ) );
}

size_t TriFloatParameter::GetValueSize() const
{
	return sizeof(m_value);
}

void TriFloatParameter::RebuildEffectHandles( ITr2ShaderState* effectRes )
{
	m_cachedEffect = effectRes;

	if ( m_name.empty() || !effectRes || !effectRes->GetConstant( m_name.c_str() ) )
	{
		m_isUsedByEffect = false;
		return;
	}

	m_isUsedByEffect = true;
}

TriAniFloatParameter::TriAniFloatParameter(IRoot* lockobj):
	TriFloatParameter( lockobj )
{
}


TriAniFloatParameter::~TriAniFloatParameter()
{
}

