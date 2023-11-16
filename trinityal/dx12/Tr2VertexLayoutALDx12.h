////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2VertexLayoutAL.h"

struct Tr2ShaderPipelineInputAL;

namespace TrinityALImpl
{
	class Tr2VertexLayoutAL : public Tr2DeviceResourceAL<Tr2VertexLayoutAL>
	{
	public:
		Tr2VertexLayoutAL();
		~Tr2VertexLayoutAL();

		ALResult Create( const Tr2VertexDefinition& definition, Tr2PrimaryRenderContextAL& renderContext );
		bool IsValid() const;
		void Destroy();

		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		Tr2VertexLayoutAL( const Tr2VertexLayoutAL& )/* = delete */;
		Tr2VertexLayoutAL& operator=( const Tr2VertexLayoutAL& )/* = delete */;

		void PopulateInputLayout( std::vector<D3D12_INPUT_ELEMENT_DESC>& layout, const std::vector<Tr2ShaderPipelineInputAL>& shaderInputs ) const;

		uint32_t m_vertexStreamMask;
		std::vector<D3D12_INPUT_ELEMENT_DESC> m_elements;
		Tr2PrimaryRenderContextAL* m_owner;
		std::string m_name;

		friend class Tr2RenderContextAL;
		friend class PSODescription;
	};
}

#endif
