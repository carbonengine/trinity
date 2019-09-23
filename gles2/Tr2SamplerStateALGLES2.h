#pragma once

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "../include/Tr2SamplerStateAL.h"

namespace TrinityALImpl
{
	class Tr2TextureAL;

	class Tr2SamplerStateAL : public Tr2DeviceResourceAL<Tr2SamplerStateAL>
	{
	public:
		Tr2SamplerStateAL();
		ALResult Create( const Tr2SamplerDescription& description, Tr2RenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;

		Tr2ALMemoryType GetMemoryClass() const;

		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	private:
		friend class ::Tr2RenderContextAL;
		friend class Tr2TextureAL;
		friend class Tr2ResourceSetAL;

		struct StateData
		{
			unsigned int m_hash;
			GLint m_magFilter;
			GLint m_minFilter;
			GLint m_minFilterNoMips;
			GLint m_wrapT;
			GLint m_wrapS;
			GLint m_wrapR;
			int m_anisotropy;
#if !defined(TRINITY_AL_MOBILE)
			float m_minLod;
			float m_maxLod;
#endif
		};

		static ALResult CreateStateData( const Tr2SamplerDescription& description, StateData& stateData );
		static ALResult Apply( GLenum textureType, bool hasMipLevels, const StateData& stateData );

		bool m_isValid;
		StateData m_stateData;
		// mip LOD bias, min/max LOD?
		// border?
	};
}

#endif
