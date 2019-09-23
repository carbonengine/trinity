#pragma once

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "../include/Tr2VertexLayoutAL.h"
#include "../Tr2VertexDefinition.h"

namespace TrinityALImpl
{
	class Tr2ShaderAL;

	class Tr2VertexLayoutAL : public Tr2DeviceResourceAL<Tr2VertexLayoutAL>
	{
	public:
		Tr2VertexLayoutAL();

		ALResult Create( const Tr2VertexDefinition& definition,
			Tr2RenderContextAL& renderContext );
		bool IsValid() const;
		void Destroy();

		ALResult SetLayout( const TrinityALImpl::Tr2ShaderAL* vertexShader, Tr2RenderContextAL& renderContext ) const;

		bool operator==( const Tr2VertexLayoutAL& other ) const
		{
			return this == &other;
		}

		Tr2ALMemoryType GetMemoryClass() const
		{
			return AL_MEMORY_MANAGED;
		}
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	private:
		Tr2VertexLayoutAL( const Tr2VertexLayoutAL& )/* = delete */;
		Tr2VertexLayoutAL& operator=( const Tr2VertexLayoutAL& )/* = delete */;

		std::unique_ptr<Tr2VertexDefinition> m_definition;
		friend class ::Tr2RenderContextAL;
	};
}
#endif // DX9?
