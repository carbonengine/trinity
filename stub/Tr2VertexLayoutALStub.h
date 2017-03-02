#pragma once
#ifndef Tr2VertexLayoutALStub_h_
#define Tr2VertexLayoutALStub_h_

#if( TRINITY_PLATFORM==TRINITY_STUB )

class Tr2VertexDefinition;

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
		return m_definition.get() != nullptr;;
	}
	void Destroy();

	ALResult SetLayout( const Tr2ShaderAL* vertexShader, Tr2RenderContextAL& renderContext ) const;

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}
private:
	std::unique_ptr<Tr2VertexDefinition> m_definition;
};

#endif // STUB?

#endif // Tr2VertexLayoutStub_h_
