#include "StdAfx.h"
#include "TriFloatArrayParameter.h"
#include "ITr2ShaderState.h"

// ------------------------------------------------------------------------------------------------------
TriVector4::TriVector4( IRoot* lockobj ) :
	m_data( 0.f, 0.f, 0.f, 0.f )
{
}

// ------------------------------------------------------------------------------------------------------
TriVector4::~TriVector4()
{

}




// ------------------------------------------------------------------------------------------------------
TriFloatArrayParameter::TriFloatArrayParameter( IRoot* lockobj ):
	PARENTLOCK( m_value ),
	m_isUsedByEffect( false )
{
}

// ------------------------------------------------------------------------------------------------------
TriFloatArrayParameter::~TriFloatArrayParameter()
{
	return;
}

// ------------------------------------------------------------------------------------------------------
bool TriFloatArrayParameter::OnModified( Be::Var* val )
{
	RebuildEffectHandles( m_cachedEffect );

	return true;
}

// ------------------------------------------------------------------------------------------------------
const char* TriFloatArrayParameter::GetParameterName() const
{
	return m_name.c_str();
}

// --------------------------------------------------------------------------------------
// Description:
//   Determines whether the length of all vector4 parameters in the array is 0 and so the
//   entire parameter array can be ignored when building the material situation.
// Return Value:
//   true, if the length of the every vector4 in the array is within epsilon of 0
//   false, otherwise
// --------------------------------------------------------------------------------------
bool TriFloatArrayParameter::IsZeroOrNull( void ) const
{
	static const float epsilon = 0.0000001f;

	for( PTriVector4Vector::const_iterator it = m_value.begin(); it != m_value.end(); ++it )
	{
		const Vector4& vec = (*it)->m_data;
		if( XMVectorGetX( XMVector4Length( vec ) ) > epsilon )
		{
			return false;
		}
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
void TriFloatArrayParameter::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
												unsigned char* destHandle, 
												size_t size,
												Tr2RenderContext &renderContext ) const
{
	unsigned char* pDst = destHandle;
	size_t bytesCopied = 0;
	for( PTriVector4Vector::const_iterator it = m_value.begin(); it != m_value.end(); ++it )
	{
		memcpy( pDst, &((*it)->m_data), sizeof( Vector4 ) );
		pDst += sizeof( Vector4 );
		bytesCopied += sizeof( Vector4 );
		if( bytesCopied >= size )
		{
			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------------
size_t TriFloatArrayParameter::GetValueSize() const
{
	return m_value.size() * sizeof( TriVector4 );
}

// ------------------------------------------------------------------------------------------------------
void TriFloatArrayParameter::RebuildEffectHandles( ITr2ShaderState* effectRes )
{
	m_cachedEffect = effectRes;
	if ( m_name.empty() || !effectRes || !effectRes->GetConstant( m_name.c_str() ) )
	{
		m_isUsedByEffect = false;
		return;
	}

	m_isUsedByEffect = true;
}
