#include "StdAfx.h"
#include "TriVector4Parameter.h"
#include "ITr2ShaderState.h"

TriVector4Parameter::TriVector4Parameter(IRoot* lockobj):
	m_isUsedByEffect( false ),
	m_isSrgb( false )
{
	m_value = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
}

bool TriVector4Parameter::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_name ) )
	{
		RebuildEffectHandles( m_cachedEffect );
	}

	return true;
}

TriVector4Parameter::~TriVector4Parameter()
{
	return;
}

const char* TriVector4Parameter::GetParameterName() const
{
	return m_name.c_str();
}

// --------------------------------------------------------------------------------------
// Description:
//   Determines whether the length of this vector4 parameter is 0 and be ignored when
//   building the material situation.
// Return Value:
//   true, if the length of the vector4 value is within epsilon of 0
//   false, otherwise
// --------------------------------------------------------------------------------------
bool TriVector4Parameter::IsZeroOrNull( void ) const
{
	static const float epsilon = 0.0000001f;

	return XMVectorGetX( XMVector4Length( m_value ) ) < epsilon;
}

void TriVector4Parameter::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
												unsigned char* destHandle, 
												size_t size,
												Tr2RenderContext &renderContext ) const
{
	if( m_isSrgb )
	{
		Vector4 linearValue = TriGammaToLinear( m_value );

		memcpy( destHandle, &linearValue, size < sizeof(m_value) ? size : sizeof( m_value ) );
		return;
	}

	memcpy( destHandle, &m_value, size < sizeof(m_value) ? size : sizeof( m_value ) );
}

size_t TriVector4Parameter::GetValueSize() const
{
	return sizeof(m_value);
}

void TriVector4Parameter::RebuildEffectHandles( ITr2ShaderState* effectRes )
{
	m_cachedEffect = effectRes;

	if ( m_name.empty() || !effectRes )
	{
		m_isUsedByEffect = false;
		return;
	}

	const Tr2EffectConstant* constant = effectRes->GetConstant( m_name.c_str() );

	if ( !constant )
	{
		m_isUsedByEffect = false;
		return;
	}

	m_isUsedByEffect = true;

	m_isSrgb = constant->isSRGB;
}
