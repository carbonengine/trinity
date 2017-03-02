#include "StdAfx.h"
#include "Tr2VertexLayoutALDx9.h"

#include "Tr2VertexDefinition.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

using namespace Tr2RenderContextEnum;

namespace {

	D3DDECLUSAGE usageNames[ Tr2VertexDefinition::NUM_USAGE_CODE ] = 
	{
		D3DDECLUSAGE_POSITION,
		D3DDECLUSAGE_COLOR,
		D3DDECLUSAGE_NORMAL,
		D3DDECLUSAGE_TANGENT,
		D3DDECLUSAGE_BINORMAL,
		D3DDECLUSAGE_TEXCOORD,
		D3DDECLUSAGE_BLENDINDICES,
		D3DDECLUSAGE_BLENDWEIGHT
	};

	D3DDECLTYPE GetD3dDataType( Tr2VertexDefinition::DataType dataType )
	{
		typedef Tr2VertexDefinition tvd;
		switch( dataType )
		{
		case tvd::FLOAT32_1		: return D3DDECLTYPE_FLOAT1;	// 1D float expanded to (value, 0., 0., 1.)
		case tvd::FLOAT32_2		: return D3DDECLTYPE_FLOAT2;	// 2D float expanded to (value, value, 0., 1.)
		case tvd::FLOAT32_3		: return D3DDECLTYPE_FLOAT3;	// 3D float expanded to (value, value, value, 1.)
		case tvd::FLOAT32_4		: return D3DDECLTYPE_FLOAT4;	// 4D float
		case tvd::UBYTE_4_NORM	: return D3DDECLTYPE_UBYTE4N;
		case tvd::UBYTE_4		: return D3DDECLTYPE_UBYTE4;  // 4D unsigned byte
		case tvd::SHORT_2		: return D3DDECLTYPE_SHORT2;  // 2D signed short expanded to (value, value, 0., 1.)
		case tvd::SHORT_4		: return D3DDECLTYPE_SHORT4;  // 4D signed short

		// emulate old GeometryUtils behavior that is not 100% accurate, but it's the closest we got
		case tvd::USHORT_4		: return D3DDECLTYPE_SHORT4;
		// GrannyInt8Member in the past would get mapped to UBYTE_4; today that becomes the corret BYTE_4, so emulate the old behavior
		case tvd::BYTE_4		:  return D3DDECLTYPE_UBYTE4;

		//D3DDECLTYPE_UBYTE4N   =  8,  // Each of 4 bytes is normalized by dividing to 255.0
		case tvd::SHORT_2_NORM	: return D3DDECLTYPE_SHORT2N;  // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)		
		case tvd::SHORT_4_NORM	: return D3DDECLTYPE_SHORT4N;  // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
		case tvd::USHORT_2_NORM	: return D3DDECLTYPE_USHORT2N;  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)		
		case tvd::USHORT_4_NORM	: return D3DDECLTYPE_USHORT4N; // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)		
		case tvd::FLOAT16_2		: return D3DDECLTYPE_FLOAT16_2;  // Two 16-bit floating point values, expanded to (value, value, 0, 1)
		case tvd::FLOAT16_4		: return D3DDECLTYPE_FLOAT16_4;  // Four 16-bit floating point values
		}

		return D3DDECLTYPE_UNUSED;
	};
}

ALResult Tr2VertexLayoutAL::Create( const Tr2VertexDefinition& definition, Tr2RenderContextAL& renderContext )
{
	if( !renderContext.m_d3dDevice9 || definition.m_items.empty() )
	{
		return E_FAIL;
	}

	std::vector<D3DVERTEXELEMENT9> desc( definition.m_items.size() );

	for( size_t i = 0; i != definition.m_items.size(); ++i )
    {
		const Tr2VertexDefinition::Item& src = definition.m_items[i];
		D3DVERTEXELEMENT9& dst = desc[i];
		
		
		dst.Method = D3DDECLMETHOD_DEFAULT;		
		dst.Offset     = static_cast<WORD>( src.m_offset );
		dst.Stream     = static_cast<WORD>( src.m_stream );
		dst.Usage      = static_cast<BYTE>( usageNames[ src.m_usage ] );
		dst.Type       = static_cast<BYTE>( GetD3dDataType( src.m_dataType ) );
		dst.UsageIndex = static_cast<BYTE>( src.m_usageIndex );

		CCP_ASSERT( dst.Type != D3DDECLTYPE_UNUSED );
    };

	D3DVERTEXELEMENT9 end = D3DDECL_END();
	desc.push_back( end );

	HRESULT hr = renderContext.m_d3dDevice9->CreateVertexDeclaration( desc.data(), &m_layout );
	if( SUCCEEDED( hr ) )
	{
		ChangeObjectId();
	}
	return hr;
}

void Tr2VertexLayoutAL::Destroy()
{
	m_layout = nullptr;
}

ALResult Tr2VertexLayoutAL::SetLayout( const Tr2ShaderAL* /*vertexShader*/, Tr2RenderContextAL& renderContext ) const
{
	if( m_layout == nullptr || renderContext.m_d3dDevice9 == nullptr )
	{
		return E_FAIL;
	}
	return renderContext.m_d3dDevice9->SetVertexDeclaration( m_layout );
}

#endif // DX9?
