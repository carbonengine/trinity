#pragma once


#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "../include/Tr2VertexLayoutAL.h"

namespace TrinityALImpl
{
	class Tr2ShaderAL;

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
			return m_layout != nullptr;
		}
		void Destroy();

		ALResult SetLayout( const TrinityALImpl::Tr2ShaderAL* vertexShader, Tr2RenderContextAL& renderContext ) const;

		Tr2ALMemoryType GetMemoryClass() const
		{
			return AL_MEMORY_MANAGED;
		}
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	private:
		Tr2VertexLayoutAL( const Tr2VertexLayoutAL& )/* = delete */;
		Tr2VertexLayoutAL& operator=( const Tr2VertexLayoutAL& )/* = delete */;

		CComPtr<IDirect3DVertexDeclaration9> m_layout;
	};
}

#endif // DX9?
