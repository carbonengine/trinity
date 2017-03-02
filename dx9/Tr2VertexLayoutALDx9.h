#pragma once
#ifndef Tr2VertexLayoutALDx9_h_
#define Tr2VertexLayoutALDx9_h_

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

class Tr2VertexDefinition;

// -------------------------------------------------------------
// Description
//   Class to convert a platform agnostic Tr2VertexDeclaration to a DX9 specific
//   declaration, and build a ID3D9InputLayout out of it.
// -------------------------------------------------------------

class Tr2VertexLayoutAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_VERTEX_LAYOUT>
{
public:
	Tr2VertexLayoutAL()
	{
	}

	ALResult Create( const Tr2VertexDefinition& definition,
					 Tr2RenderContextAL& renderContext );
	bool IsValid() const
	{
		return m_layout != nullptr;
	}
	void Destroy();

	ALResult SetLayout( const Tr2ShaderAL* vertexShader, Tr2RenderContextAL& renderContext ) const;

	bool operator==( const Tr2VertexLayoutAL& other ) const
	{
		return m_layout == other.m_layout;
	}

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}
private:
	Tr2VertexLayoutAL( const Tr2VertexLayoutAL& )/* = delete */;
	Tr2VertexLayoutAL& operator=( const Tr2VertexLayoutAL& )/* = delete */;

	CComPtr<IDirect3DVertexDeclaration9> m_layout;
};

#endif // DX9?

#endif // Tr2VertexLayoutDx9_h_
