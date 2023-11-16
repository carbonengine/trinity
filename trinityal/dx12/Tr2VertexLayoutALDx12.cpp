////////////////////////////////////////////////////////////
//
//    Created:   March 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2VertexLayoutALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Tr2VertexDefinition.h"
#include "Tr2ShaderALDx12.h"

namespace
{
	const char* s_usageNames[] = {
		"POSITION",
		"COLOR",
		"NORMAL",
		"TANGENT",
		"BINORMAL",
		"TEXCOORD",
		"BLENDINDICES",
		"BLENDWEIGHT",
	};
	static_assert( sizeof( s_usageNames ) / sizeof( s_usageNames[0] ) == Tr2VertexDefinition::NUM_USAGE_CODE, "Usage name count mismatch" );

	DXGI_FORMAT GetDxgiDataType( Tr2VertexDefinition::DataType dataType )
	{

#define VD_CASE(x,y) case Tr2VertexDefinition::x : return y;
		switch( dataType )
		{
			VD_CASE( BYTE_1, DXGI_FORMAT_R8_SINT );
			VD_CASE( BYTE_2, DXGI_FORMAT_R8G8_SINT );
			VD_CASE( BYTE_3, DXGI_FORMAT_UNKNOWN );
			VD_CASE( BYTE_4, DXGI_FORMAT_R8G8B8A8_SINT );

			VD_CASE( UBYTE_1, DXGI_FORMAT_R8_UINT );
			VD_CASE( UBYTE_2, DXGI_FORMAT_R8G8_UINT );
			VD_CASE( UBYTE_3, DXGI_FORMAT_UNKNOWN );
			VD_CASE( UBYTE_4, DXGI_FORMAT_R8G8B8A8_UINT );

			VD_CASE( BYTE_1_NORM, DXGI_FORMAT_R8_SNORM );
			VD_CASE( BYTE_2_NORM, DXGI_FORMAT_R8G8_SNORM );
			VD_CASE( BYTE_3_NORM, DXGI_FORMAT_UNKNOWN );
			VD_CASE( BYTE_4_NORM, DXGI_FORMAT_R8G8B8A8_SNORM );

			VD_CASE( UBYTE_1_NORM, DXGI_FORMAT_R8_UNORM );
			VD_CASE( UBYTE_2_NORM, DXGI_FORMAT_R8G8_UNORM );
			VD_CASE( UBYTE_3_NORM, DXGI_FORMAT_UNKNOWN );
			VD_CASE( UBYTE_4_NORM, DXGI_FORMAT_R8G8B8A8_UNORM );


			VD_CASE( SHORT_1, DXGI_FORMAT_R16_SINT );
			VD_CASE( SHORT_2, DXGI_FORMAT_R16G16_SINT );
			VD_CASE( SHORT_3, DXGI_FORMAT_UNKNOWN );
			VD_CASE( SHORT_4, DXGI_FORMAT_R16G16B16A16_SINT );

			VD_CASE( USHORT_1, DXGI_FORMAT_R16_UINT );
			VD_CASE( USHORT_2, DXGI_FORMAT_R16G16_UINT );
			VD_CASE( USHORT_3, DXGI_FORMAT_UNKNOWN );
			VD_CASE( USHORT_4, DXGI_FORMAT_R16G16B16A16_UINT );

			VD_CASE( SHORT_1_NORM, DXGI_FORMAT_R16_SNORM );
			VD_CASE( SHORT_2_NORM, DXGI_FORMAT_R16G16_SNORM );
			VD_CASE( SHORT_3_NORM, DXGI_FORMAT_UNKNOWN );
			VD_CASE( SHORT_4_NORM, DXGI_FORMAT_R16G16B16A16_SNORM );

			VD_CASE( USHORT_1_NORM, DXGI_FORMAT_R16_UNORM );
			VD_CASE( USHORT_2_NORM, DXGI_FORMAT_R16G16_UNORM );
			VD_CASE( USHORT_3_NORM, DXGI_FORMAT_UNKNOWN );
			VD_CASE( USHORT_4_NORM, DXGI_FORMAT_R16G16B16A16_UNORM );


			VD_CASE( INT32_1, DXGI_FORMAT_R32_SINT );
			VD_CASE( INT32_2, DXGI_FORMAT_R32G32_SINT );
			VD_CASE( INT32_3, DXGI_FORMAT_R32G32B32_SINT );
			VD_CASE( INT32_4, DXGI_FORMAT_R32G32B32A32_SINT );

			VD_CASE( UINT32_1, DXGI_FORMAT_R32_UINT );
			VD_CASE( UINT32_2, DXGI_FORMAT_R32G32_UINT );
			VD_CASE( UINT32_3, DXGI_FORMAT_R32G32B32_UINT );
			VD_CASE( UINT32_4, DXGI_FORMAT_R32G32B32A32_UINT );


			VD_CASE( FLOAT16_1, DXGI_FORMAT_R16_FLOAT );
			VD_CASE( FLOAT16_2, DXGI_FORMAT_R16G16_FLOAT );
			VD_CASE( FLOAT16_3, DXGI_FORMAT_UNKNOWN );
			VD_CASE( FLOAT16_4, DXGI_FORMAT_R16G16B16A16_FLOAT );

			VD_CASE( FLOAT32_1, DXGI_FORMAT_R32_FLOAT );
			VD_CASE( FLOAT32_2, DXGI_FORMAT_R32G32_FLOAT );
			VD_CASE( FLOAT32_3, DXGI_FORMAT_R32G32B32_FLOAT );
			VD_CASE( FLOAT32_4, DXGI_FORMAT_R32G32B32A32_FLOAT );
		}

		return DXGI_FORMAT_UNKNOWN;
	};
}

namespace TrinityALImpl
{

	Tr2VertexLayoutAL::Tr2VertexLayoutAL() :
		m_vertexStreamMask( 0 ),
		m_owner( nullptr )
	{
	}

	Tr2VertexLayoutAL::~Tr2VertexLayoutAL()
	{
		Destroy();
	}

	ALResult Tr2VertexLayoutAL::Create( const Tr2VertexDefinition& definition, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		for( auto it = begin( definition.m_items ); it != end( definition.m_items ); ++it )
		{
			D3D12_INPUT_ELEMENT_DESC element;
			element.SemanticName = s_usageNames[it->m_usage];
			element.SemanticIndex = it->m_usageIndex;
			element.Format = GetDxgiDataType( it->m_dataType );
			element.InputSlot = it->m_stream;
			element.AlignedByteOffset = it->m_offset;
			element.InputSlotClass = it->m_instanceStepRate ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			element.InstanceDataStepRate = it->m_instanceStepRate;
			m_elements.push_back( element );

			m_vertexStreamMask |= 1 << it->m_stream;
		}

		m_owner = &renderContext;

		return S_OK;
	}

	bool Tr2VertexLayoutAL::IsValid() const
	{
		return m_owner != nullptr;
	}

	void Tr2VertexLayoutAL::Destroy()
	{
		if( m_owner )
		{
			m_owner->OnVertexLayoutDestroyedDx12( this );
		}
		m_elements.clear();
		m_owner = nullptr;
		m_vertexStreamMask = 0;
	}

	Tr2ALMemoryType Tr2VertexLayoutAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	void Tr2VertexLayoutAL::PopulateInputLayout( std::vector<D3D12_INPUT_ELEMENT_DESC>& layout, const std::vector<Tr2ShaderPipelineInputAL>& shaderInputs ) const
	{
		layout.reserve( shaderInputs.size() );
		for( auto it = begin( shaderInputs ); it != end( shaderInputs ); ++it )
		{
			auto& in = *it;
			bool found = false;
			for( auto jt = begin( m_elements ); jt != end( m_elements ); ++jt )
			{
				auto out = *jt;
				if( in.usageIndex == out.SemanticIndex && s_usageNames[in.usage] == out.SemanticName )
				{
					layout.push_back( *jt );
					found = true;
					break;
				}
			}
			if( !found )
			{
				D3D12_INPUT_ELEMENT_DESC desc = { s_usageNames[in.usage], in.usageIndex, it->usage == Tr2VertexDefinition::BLENDINDICES ? DXGI_FORMAT_R8G8B8A8_UINT : DXGI_FORMAT_R32G32B32A32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
				layout.push_back( desc );
			}
		}
	}

	void Tr2VertexLayoutAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2VertexLayoutAL";
		description["name"] = m_name;
	}

	ALResult Tr2VertexLayoutAL::SetName( const char* name )
	{
		m_name = name;
		return S_OK;
	}
}
#endif