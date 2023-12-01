#pragma once

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "../include/Tr2VertexLayoutAL.h"
#include "../Tr2VertexDefinition.h"


namespace TrinityALImpl
{
	class Tr2VertexLayoutAL : public Tr2DeviceResourceAL<Tr2VertexLayoutAL>
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

		Tr2ALMemoryType GetMemoryClass() const
		{
			return AL_MEMORY_MANAGED;
		}
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		std::unique_ptr<Tr2VertexDefinition> m_definition;
	};
}

#endif