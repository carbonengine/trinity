#pragma once
#ifndef Tr2VertexLayoutALGLES2_h_
#define Tr2VertexLayoutALGLES2_h_

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "Tr2VertexDefinition.h"

#include <memory>

// -------------------------------------------------------------
// Description
//   Class to convert a platform agnostic Tr2VertexDeclaration to a DX9 specific
//   declaration, and build a ID3D9InputLayout out of it.
// -------------------------------------------------------------

class Tr2VertexLayoutAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_VERTEX_LAYOUT>
{
public:
	Tr2VertexLayoutAL();

	ALResult Create( const Tr2VertexDefinition& definition,
					 Tr2RenderContextAL& renderContext );
	bool IsValid() const;
	void Destroy();

	ALResult SetLayout( const Tr2ShaderAL* vertexShader, Tr2RenderContextAL& renderContext ) const;

	bool operator==( const Tr2VertexLayoutAL& other ) const
	{
		return this == &other;
	}

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}
private:
	Tr2VertexLayoutAL( const Tr2VertexLayoutAL& )/* = delete */;
	Tr2VertexLayoutAL& operator=( const Tr2VertexLayoutAL& )/* = delete */;

	std::unique_ptr<Tr2VertexDefinition> m_definition;
	friend class Tr2RenderContextAL;
};

#endif // DX9?

#endif // Tr2VertexLayoutGLES2_h_
