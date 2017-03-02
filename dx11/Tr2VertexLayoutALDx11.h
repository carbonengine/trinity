#pragma once
#ifndef Tr2VertexLayoutALDx11_h_
#define Tr2VertexLayoutALDx11_h_

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

class Tr2VertexDefinition;
class Tr2ShaderAL;

// -------------------------------------------------------------
// Description
//   Class to convert a platform agnostic Tr2VertexDeclaration to a DX11 specific
//   declaration, and build a ID3D11InputLayout out of it.
//   Not intended to be used directly, instead use Tr2VertexDefinition, apply
//   the definition to the Tr2EffectStateManager, and let things happen behind the scenes.
// -------------------------------------------------------------

class Tr2VertexLayoutAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_VERTEX_LAYOUT>
{
public:
	Tr2VertexLayoutAL()
	: m_definition( "Tr2VertexLayoutAL::m_definition" )
	{
	}

	ALResult Create( const Tr2VertexDefinition& definition,
					 Tr2RenderContextAL& renderContext );
	bool IsValid() const
	{
		return !m_definition.empty();
	}
	void Destroy();

	ALResult SetLayout( const Tr2ShaderAL* vertexShader, Tr2RenderContextAL& renderContext ) const;

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

private:
	Tr2VertexLayoutAL( const Tr2VertexLayoutAL& )/* = delete */;
	Tr2VertexLayoutAL& operator=( const Tr2VertexLayoutAL& )/* = delete */;

	TrackableStdVector<D3D11_INPUT_ELEMENT_DESC> m_definition;
	mutable std::map<unsigned, CComPtr<ID3D11InputLayout>> m_layout;
};

#endif // DX11?

#endif // Tr2VertexLayoutDx11_h_
