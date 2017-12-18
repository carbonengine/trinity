#pragma once
#ifndef Tr2SamplerStateALDx9_H
#define Tr2SamplerStateALDx9_H

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"


class Tr2RenderContextAL;
struct Tr2SamplerDescription;


#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

namespace TrinityALImpl
{
	class Tr2SamplerStateAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SAMPLER_STATE>
	{
	public:
		Tr2SamplerStateAL();
		~Tr2SamplerStateAL();

		ALResult Create( const Tr2SamplerDescription& description, Tr2RenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;

		Tr2ALMemoryType GetMemoryClass() const;

	private:
		friend class ::Tr2RenderContextAL;
		friend class Tr2ResourceSetAL;

		GLuint m_sampler;

		struct StateData
		{
			unsigned int m_hash;
			GLint m_magFilter;
			GLint m_minFilter;
			GLint m_wrapT;
			GLint m_wrapS;
			GLint m_wrapR;
			int m_anisotropy;
			float m_minLod;
			float m_maxLod;
		};

		static ALResult CreateStateData( const Tr2SamplerDescription& description, StateData& stateData );
		//static ALResult Apply( GLenum textureType, bool hasMipLevels, const StateData& stateData );

		bool m_isValid;
		StateData m_stateData;
		mutable cl_sampler m_clObject;
		// mip LOD bias, min/max LOD?
		// border?
	};
}

#endif

#endif
