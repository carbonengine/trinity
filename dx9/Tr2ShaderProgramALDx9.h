#pragma once

#if TRINITY_PLATFORM==TRINITY_DIRECTX9

#include "../include/Tr2ShaderProgramAL.h"
#include "../include/Tr2ShaderAL.h"

namespace TrinityALImpl
{
	class Tr2ShaderProgramAL : public Tr2DeviceResourceAL<Tr2ShaderProgramAL>
	{
	public:
		Tr2ShaderProgramAL();

		ALResult Create( ::Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;

		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	private:
		::Tr2ShaderAL m_vertexShader;
		::Tr2ShaderAL m_pixelShader;
		bool m_isValid;

		friend class Tr2RenderContextAL;
	};
}

#endif