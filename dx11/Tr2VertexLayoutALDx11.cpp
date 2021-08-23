#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2VertexLayoutALDx11.h"
#include "Tr2VertexDefinition.h"
#include "ALLog.h"
#include "Tr2RenderContextDx11.h"
#include "Tr2PrimaryRenderContextDx11.h"
#include "Tr2ShaderALDx11.h"

using namespace Tr2RenderContextEnum;

namespace {

	const char* semanticNames[ Tr2VertexDefinition::NUM_USAGE_CODE ] = 
	{
		"POSITION",
		"COLOR",
		"NORMAL",
		"TANGENT",
		"BINORMAL",
		"TEXCOORD",
		"BLENDINDICES",
		"BLENDWEIGHT"
	};

	DXGI_FORMAT GetDxgiDataType( Tr2VertexDefinition::DataType dataType )
	{

#define VD_CASE(x,y) case Tr2VertexDefinition::x : return y;
		switch( dataType )
		{
			VD_CASE( BYTE_1			, DXGI_FORMAT_R8_SINT );
			VD_CASE( BYTE_2			, DXGI_FORMAT_R8G8_SINT );
			VD_CASE( BYTE_3			, DXGI_FORMAT_UNKNOWN );
			VD_CASE( BYTE_4			, DXGI_FORMAT_R8G8B8A8_SINT );

			VD_CASE( UBYTE_1		, DXGI_FORMAT_R8_UINT );
			VD_CASE( UBYTE_2		, DXGI_FORMAT_R8G8_UINT );
			VD_CASE( UBYTE_3		, DXGI_FORMAT_UNKNOWN );
			VD_CASE( UBYTE_4		, DXGI_FORMAT_R8G8B8A8_UINT );

			VD_CASE( BYTE_1_NORM	, DXGI_FORMAT_R8_SNORM );
			VD_CASE( BYTE_2_NORM	, DXGI_FORMAT_R8G8_SNORM );
			VD_CASE( BYTE_3_NORM	, DXGI_FORMAT_UNKNOWN );
			VD_CASE( BYTE_4_NORM	, DXGI_FORMAT_R8G8B8A8_SNORM );

			VD_CASE( UBYTE_1_NORM	, DXGI_FORMAT_R8_UNORM );
			VD_CASE( UBYTE_2_NORM	, DXGI_FORMAT_R8G8_UNORM );
			VD_CASE( UBYTE_3_NORM	, DXGI_FORMAT_UNKNOWN );
			VD_CASE( UBYTE_4_NORM	, DXGI_FORMAT_R8G8B8A8_UNORM );


			VD_CASE( SHORT_1		, DXGI_FORMAT_R16_SINT );
			VD_CASE( SHORT_2		, DXGI_FORMAT_R16G16_SINT );
			VD_CASE( SHORT_3		, DXGI_FORMAT_UNKNOWN );
			VD_CASE( SHORT_4		, DXGI_FORMAT_R16G16B16A16_SINT );

			VD_CASE( USHORT_1		, DXGI_FORMAT_R16_UINT );
			VD_CASE( USHORT_2		, DXGI_FORMAT_R16G16_UINT );
			VD_CASE( USHORT_3		, DXGI_FORMAT_UNKNOWN );
			VD_CASE( USHORT_4		, DXGI_FORMAT_R16G16B16A16_UINT );

			VD_CASE( SHORT_1_NORM	, DXGI_FORMAT_R16_SNORM );
			VD_CASE( SHORT_2_NORM	, DXGI_FORMAT_R16G16_SNORM );
			VD_CASE( SHORT_3_NORM	, DXGI_FORMAT_UNKNOWN );
			VD_CASE( SHORT_4_NORM	, DXGI_FORMAT_R16G16B16A16_SNORM );

			VD_CASE( USHORT_1_NORM	, DXGI_FORMAT_R16_UNORM );
			VD_CASE( USHORT_2_NORM	, DXGI_FORMAT_R16G16_UNORM );
			VD_CASE( USHORT_3_NORM	, DXGI_FORMAT_UNKNOWN );
			VD_CASE( USHORT_4_NORM	, DXGI_FORMAT_R16G16B16A16_UNORM );


			VD_CASE( INT32_1		, DXGI_FORMAT_R32_SINT );
			VD_CASE( INT32_2		, DXGI_FORMAT_R32G32_SINT );
			VD_CASE( INT32_3		, DXGI_FORMAT_R32G32B32_SINT );
			VD_CASE( INT32_4		, DXGI_FORMAT_R32G32B32A32_SINT );

			VD_CASE( UINT32_1		, DXGI_FORMAT_R32_UINT );
			VD_CASE( UINT32_2		, DXGI_FORMAT_R32G32_UINT );
			VD_CASE( UINT32_3		, DXGI_FORMAT_R32G32B32_UINT );
			VD_CASE( UINT32_4		, DXGI_FORMAT_R32G32B32A32_UINT );


			VD_CASE( FLOAT16_1		, DXGI_FORMAT_R16_FLOAT );
			VD_CASE( FLOAT16_2		, DXGI_FORMAT_R16G16_FLOAT );
			VD_CASE( FLOAT16_3		, DXGI_FORMAT_UNKNOWN );
			VD_CASE( FLOAT16_4		, DXGI_FORMAT_R16G16B16A16_FLOAT );

			VD_CASE( FLOAT32_1		, DXGI_FORMAT_R32_FLOAT );
			VD_CASE( FLOAT32_2		, DXGI_FORMAT_R32G32_FLOAT );
			VD_CASE( FLOAT32_3		, DXGI_FORMAT_R32G32B32_FLOAT );
			VD_CASE( FLOAT32_4		, DXGI_FORMAT_R32G32B32A32_FLOAT );
		}

		return DXGI_FORMAT_UNKNOWN;
	};
}

namespace TrinityALImpl
{

	ALResult Tr2VertexLayoutAL::Create( const Tr2VertexDefinition& definition, Tr2PrimaryRenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() )
		{
			return E_FAIL;
		}

		m_definition.resize( definition.m_items.size() );

		for( size_t i = 0; i != definition.m_items.size(); ++i )
		{
			const Tr2VertexDefinition::Item& src = definition.m_items[i];
			D3D11_INPUT_ELEMENT_DESC& dst = m_definition[i];

			dst.SemanticName = semanticNames[src.m_usage];
			dst.SemanticIndex = src.m_usageIndex;

			dst.Format = GetDxgiDataType( src.m_dataType );
			CCP_ASSERT( dst.Format != DXGI_FORMAT_UNKNOWN );

			dst.InputSlot = src.m_stream;
			dst.AlignedByteOffset = src.m_offset;
			dst.InputSlotClass = src.m_instanceStepRate ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
			dst.InstanceDataStepRate = src.m_instanceStepRate;
		};

		return S_OK;
	}

	void Tr2VertexLayoutAL::Destroy()
	{
		m_layout.clear();
		m_definition.clear();
	}

	static bool FindInputElement( const TrackableStdVector<D3D11_INPUT_ELEMENT_DESC>& elements,
		const Tr2ShaderPipelineInputAL& element )
	{
		for( auto it = elements.begin(); it != elements.end(); ++it )
		{
			if( element.usageIndex == it->SemanticIndex && strcmp( semanticNames[element.usage], it->SemanticName ) == 0 )
			{
				return true;
			}
		}
		return false;
	}

	ALResult Tr2VertexLayoutAL::SetLayout( const TrinityALImpl::Tr2ShaderAL* vertexShader, Tr2RenderContextAL& renderContext ) const
	{
		if( !renderContext.m_secondaryDevice11 || !vertexShader || m_definition.empty() )
		{
			return E_FAIL;
		}
		auto found = m_layout.find( vertexShader->m_pipelineInputHash );
		if( found != m_layout.end() )
		{
			renderContext.m_context->IASetInputLayout( found->second );
			return S_OK;
		}

		Tr2ShaderBytecodeAL bytecode;
		CR_RETURN_HR( vertexShader->GetBytecode( bytecode ) );

		const auto& definition = vertexShader->m_signature;
		auto patchedDefinition = m_definition;
		for( auto it = definition.pipelineInputs.begin(); it != definition.pipelineInputs.end(); ++it )
		{
			if( !FindInputElement( m_definition, *it ) )
			{
				D3D11_INPUT_ELEMENT_DESC fakeElement;
				fakeElement.AlignedByteOffset = 0;
				switch( it->type )
				{
				case Tr2ShaderPipelineInputAL::INT:
					fakeElement.Format = DXGI_FORMAT_R8G8B8A8_SINT;
					break;
				case Tr2ShaderPipelineInputAL::UINT:
					fakeElement.Format = DXGI_FORMAT_R8G8B8A8_UINT;
					break;
				default:
					fakeElement.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				}
				fakeElement.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
				fakeElement.InstanceDataStepRate = 0;
				fakeElement.SemanticIndex = it->usageIndex;
				fakeElement.SemanticName = semanticNames[it->usage];
				if( it->usedMask == 0 )
				{
					fakeElement.InputSlot = 4;
				}
				else
				{
					fakeElement.InputSlot = 4;
				}
				patchedDefinition.push_back( fakeElement );
			}
		}

		CComPtr<ID3D11InputLayout> layout;
		long result = renderContext.m_secondaryDevice11->CreateInputLayout(
			patchedDefinition.data(),
			(uint32_t)patchedDefinition.size(),
			bytecode.bytecode,
			bytecode.size,
			&layout );

		if( result == S_OK )
		{
			m_layout[vertexShader->m_pipelineInputHash] = layout;
			renderContext.m_context->IASetInputLayout( layout );
		}
		return result;
	}

	void Tr2VertexLayoutAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2VertexLayoutAL";
	}
}

#endif // DX11?
