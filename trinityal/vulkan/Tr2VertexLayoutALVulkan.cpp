#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2VertexLayoutALVulkan.h"

namespace
{
	VkFormat GetFormat( Tr2VertexDefinition::DataType vertexType )
	{
#define VD_CASE(x,y) case Tr2VertexDefinition::x : return y;
		switch( vertexType )
		{
			VD_CASE( BYTE_1, VK_FORMAT_R8_SINT );
			VD_CASE( BYTE_2, VK_FORMAT_R8G8_SINT );
			VD_CASE( BYTE_3, VK_FORMAT_R8G8B8_SINT );
			VD_CASE( BYTE_4, VK_FORMAT_R8G8B8A8_SINT );

			VD_CASE( UBYTE_1, VK_FORMAT_R8_UINT );
			VD_CASE( UBYTE_2, VK_FORMAT_R8G8_UINT );
			VD_CASE( UBYTE_3, VK_FORMAT_R8G8B8_UINT );
			VD_CASE( UBYTE_4, VK_FORMAT_R8G8B8A8_UINT );


			VD_CASE( BYTE_1_NORM, VK_FORMAT_R8_SNORM );
			VD_CASE( BYTE_2_NORM, VK_FORMAT_R8G8_SNORM );
			VD_CASE( BYTE_3_NORM, VK_FORMAT_R8G8B8_SNORM );
			VD_CASE( BYTE_4_NORM, VK_FORMAT_R8G8B8A8_SNORM );

			VD_CASE( UBYTE_1_NORM, VK_FORMAT_R8_UNORM );
			VD_CASE( UBYTE_2_NORM, VK_FORMAT_R8G8_UNORM );
			VD_CASE( UBYTE_3_NORM, VK_FORMAT_R8G8B8_UNORM );
			VD_CASE( UBYTE_4_NORM, VK_FORMAT_R8G8B8A8_UNORM );

			VD_CASE( SHORT_1, VK_FORMAT_R16_SINT );
			VD_CASE( SHORT_2, VK_FORMAT_R16G16_SINT );
			VD_CASE( SHORT_3, VK_FORMAT_R16G16B16_SINT );
			VD_CASE( SHORT_4, VK_FORMAT_R16G16B16A16_SINT );

			VD_CASE( USHORT_1, VK_FORMAT_R16_UINT );
			VD_CASE( USHORT_2, VK_FORMAT_R16G16_UINT );
			VD_CASE( USHORT_3, VK_FORMAT_R16G16B16_UINT );
			VD_CASE( USHORT_4, VK_FORMAT_R16G16B16A16_UINT );

			VD_CASE( SHORT_1_NORM, VK_FORMAT_R16_SNORM );
			VD_CASE( SHORT_2_NORM, VK_FORMAT_R16G16_SNORM );
			VD_CASE( SHORT_3_NORM, VK_FORMAT_R16G16B16_SNORM );
			VD_CASE( SHORT_4_NORM, VK_FORMAT_R16G16B16A16_SNORM );

			VD_CASE( USHORT_1_NORM, VK_FORMAT_R16_UNORM );
			VD_CASE( USHORT_2_NORM, VK_FORMAT_R16G16_UNORM );
			VD_CASE( USHORT_3_NORM, VK_FORMAT_R16G16B16_UNORM );
			VD_CASE( USHORT_4_NORM, VK_FORMAT_R16G16B16A16_UNORM );

			VD_CASE( INT32_1, VK_FORMAT_R32_SINT );
			VD_CASE( INT32_2, VK_FORMAT_R32G32_SINT );
			VD_CASE( INT32_3, VK_FORMAT_R32G32B32_SINT );
			VD_CASE( INT32_4, VK_FORMAT_R32G32B32A32_SINT );

			VD_CASE( UINT32_1, VK_FORMAT_R32_UINT );
			VD_CASE( UINT32_2, VK_FORMAT_R32G32_UINT );
			VD_CASE( UINT32_3, VK_FORMAT_R32G32B32_UINT );
			VD_CASE( UINT32_4, VK_FORMAT_R32G32B32A32_UINT );

			VD_CASE( FLOAT16_1, VK_FORMAT_R16_SFLOAT );
			VD_CASE( FLOAT16_2, VK_FORMAT_R16G16_SFLOAT );
			VD_CASE( FLOAT16_3, VK_FORMAT_R16G16B16_SFLOAT );
			VD_CASE( FLOAT16_4, VK_FORMAT_R16G16B16A16_SFLOAT );

			VD_CASE( FLOAT32_1, VK_FORMAT_R32_SFLOAT );
			VD_CASE( FLOAT32_2, VK_FORMAT_R32G32_SFLOAT );
			VD_CASE( FLOAT32_3, VK_FORMAT_R32G32B32_SFLOAT );
			VD_CASE( FLOAT32_4, VK_FORMAT_R32G32B32A32_SFLOAT );
		}

		return VK_FORMAT_UNDEFINED;
	}
}

namespace TrinityALImpl
{
	Tr2VertexLayoutAL::Tr2VertexLayoutAL()
		:m_isValid( false )
	{

	}

	ALResult Tr2VertexLayoutAL::Create( const Tr2VertexDefinition& definition, Tr2PrimaryRenderContextAL& )
	{
		Destroy();

		for( auto it = begin( definition.m_items ); it != end( definition.m_items ); ++it )
		{
			VkVertexInputAttributeDescription desc;
			desc.binding = it->m_stream;
			desc.format = GetFormat( it->m_dataType );
			desc.offset = it->m_offset;
			m_attributes.push_back( desc );
			m_streamRates[it->m_stream] = it->m_instanceStepRate ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
			m_streamCount = std::max( m_streamCount, it->m_stream + 1 );
		}
		m_definition = definition;
		m_isValid = true;
		return S_OK;
	}

	bool Tr2VertexLayoutAL::IsValid() const
	{
		return m_isValid;
	}

	void Tr2VertexLayoutAL::Destroy()
	{
		m_streamCount = 0;
		m_isValid = false;
		m_definition = Tr2VertexDefinition();
		m_attributes.clear();
	}

	Tr2ALMemoryType Tr2VertexLayoutAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	void Tr2VertexLayoutAL::PopulateInputLayoutVulkan( std::vector<VkVertexInputAttributeDescription>& layout, const std::vector<Tr2ShaderPipelineInputAL>& shaderInputs ) const
	{
		layout.reserve( shaderInputs.size() );
		for( auto it = begin( shaderInputs ); it != end( shaderInputs ); ++it )
		{
			auto& in = *it;
			bool found = false;
			for( size_t i = 0; i < m_attributes.size(); ++i )
			{
				auto& item = m_definition.m_items[i];
				if( in.usage == item.m_usage && in.usageIndex == item.m_usageIndex )
				{
					auto attr = m_attributes[i];
					attr.location = in.registerIndex;
					layout.push_back( attr );
					found = true;
					break;
				}
			}
			if( !found )
			{
				VkVertexInputAttributeDescription attr = { in.registerIndex, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 };
				layout.push_back( attr );
			}
		}
	}

	void Tr2VertexLayoutAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2VertexLayoutAL";
	}

	ALResult Tr2VertexLayoutAL::SetName( const char* )
	{
		return E_NOTIMPL;
	}
}
#endif