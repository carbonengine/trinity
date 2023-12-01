#include "StdAfx.h"
#include "TriFloatArrayParameter.h"
#include "Shader/Tr2Shader.h"

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

unsigned TriFloatArrayParameter::GetHashValue( unsigned startingHash ) const
{
	auto name = m_name.c_str();
	for( auto it = m_value.begin(); it != m_value.end(); ++it )
	{
		startingHash = CcpHashFNV1( &( *it )->m_data, sizeof( Vector4 ), startingHash );
	}
	return CcpHashFNV1( &name, sizeof( name ), startingHash );
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
void TriFloatArrayParameter::RebuildEffectHandles( Tr2Shader* effectRes )
{
	m_cachedEffect = effectRes;
	if ( m_name.empty() || !effectRes || !effectRes->GetConstant( m_name.c_str() ) )
	{
		m_isUsedByEffect = false;
		return;
	}

	m_isUsedByEffect = true;
}
