#pragma once
#ifndef Tr2SamplerStateALDx9_H
#define Tr2SamplerStateALDx9_H

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

class Tr2SamplerStateAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SAMPLER_STATE>
{
public:
	Tr2SamplerStateAL();
	ALResult Create(
		Tr2RenderContextAL& renderContext,
		const Tr2SamplerDescription& description );
	bool IsValid() const;

	bool operator==( const Tr2SamplerStateAL& ) const;

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

private:
	friend class Tr2RenderContextAL;
	friend class Tr2TextureAL;
	friend class Tr2RenderTargetAL;
	friend class Tr2DepthStencilAL;

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

#endif // TRINITY_PLATFORM==TRINITY_OPENGLES2

#endif // Tr2SamplerStateALDx9_H
