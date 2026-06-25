// Copyright © 2023 CCP ehf.

#pragma once


#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

#include "../include/Tr2VertexLayoutAL.h"

// -------------------------------------------------------------
// Description
//   Class to convert a platform agnostic Tr2VertexDeclaration to a DX11 specific
//   declaration, and build a ID3D11InputLayout out of it.
//   Not intended to be used directly, instead use Tr2VertexDefinition, apply
//   the definition to the Tr2EffectStateManager, and let things happen behind the scenes.
// -------------------------------------------------------------

class Tr2RenderContextAL;

namespace TrinityALImpl
{
class Tr2ShaderAL;

class Tr2VertexLayoutAL : public Tr2DeviceResourceAL<Tr2VertexLayoutAL>
{
public:
	Tr2VertexLayoutAL() :
		m_definition( "Tr2VertexLayoutAL::m_definition" )
	{
	}

	ALResult Create( const Tr2VertexDefinition& definition, Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const
	{
		return !m_definition.empty();
	}
	void Destroy();

	ALResult SetLayout( const TrinityALImpl::Tr2ShaderAL* vertexShader, Tr2RenderContextAL& renderContext ) const;

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

private:
	Tr2VertexLayoutAL( const Tr2VertexLayoutAL& ) /* = delete */;
	Tr2VertexLayoutAL& operator=( const Tr2VertexLayoutAL& ) /* = delete */;

	TrackableStdVector<D3D11_INPUT_ELEMENT_DESC> m_definition;
	mutable std::map<unsigned, CComPtr<ID3D11InputLayout>> m_layout;
	std::string m_name;
};
}

#endif // DX11?
