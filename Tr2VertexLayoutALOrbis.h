#pragma once
#ifndef Tr2VertexLayoutALOrbis_h_
#define Tr2VertexLayoutALOrbis_h_

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2VertexDefinition.h"

// -------------------------------------------------------------
// Description
//   Class to convert a platform agnostic Tr2VertexDeclaration to a DX9 specific
//   declaration, and build a ID3D9InputLayout out of it.
// -------------------------------------------------------------

class Tr2VertexLayoutAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_VERTEX_LAYOUT>
{
public:
	Tr2VertexLayoutAL();
	~Tr2VertexLayoutAL();

	ALResult Create( const Tr2VertexDefinition& definition,
					 Tr2RenderContextAL& renderContext );
	bool IsValid() const;
	void Destroy();

	ALResult SetLayout( const Tr2ShaderAL* vertexShader,
						Tr2RenderContextAL& renderContext );

	bool operator==( const Tr2VertexLayoutAL& other ) const
	{
		return this == &other;
	}

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}
private:
	enum FetchShaderType
	{
		SINGLE_DRAW,
		INSTANCED_DRAW,
	};
	struct FetchShader
	{
		void* shader;
		uint32_t modifier;
	};
	typedef std::pair<FetchShaderType, unsigned> FetchShaderKey;

	Tr2VertexLayoutAL( const Tr2VertexLayoutAL& )/* = delete */;
	Tr2VertexLayoutAL& operator=( const Tr2VertexLayoutAL& )/* = delete */;

	Tr2VertexDefinition m_definition;
	bool m_isValid;
	mutable std::map<FetchShaderKey, FetchShader> m_fetchShaders;
	mutable uint32_t m_frameUsed;

	friend class Tr2RenderContextAL;
};

#endif

#endif
