#pragma once

#if TRINITY_PLATFORM == TRINITY_STUB

#include "../include/Tr2ShaderProgramAL.h"

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
		bool m_isValid;

		friend class Tr2RenderContextAL;
	};
}

#endif