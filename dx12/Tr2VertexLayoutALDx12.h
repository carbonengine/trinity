////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../include/Tr2ShaderAL.h"


class Tr2PrimaryRenderContextAL;
class Tr2VertexDefinition;


class Tr2VertexLayoutAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_VERTEX_LAYOUT>
{
public:
	Tr2VertexLayoutAL();
	~Tr2VertexLayoutAL();

	ALResult Create( const Tr2VertexDefinition& definition, Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;
	void Destroy();

	Tr2ALMemoryType GetMemoryClass() const;
private:
	Tr2VertexLayoutAL( const Tr2VertexLayoutAL& )/* = delete */;
	Tr2VertexLayoutAL& operator=( const Tr2VertexLayoutAL& )/* = delete */;

	void PopulateInputLayout( std::vector<D3D12_INPUT_ELEMENT_DESC>& layout, const std::vector<Tr2ShaderPipelineInputAL>& shaderInputs ) const;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_elements;
	Tr2PrimaryRenderContextAL* m_owner;

	friend class Tr2RenderContextAL;
};

#endif
