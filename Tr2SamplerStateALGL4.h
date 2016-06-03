#pragma once
#ifndef Tr2SamplerStateALDx9_H
#define Tr2SamplerStateALDx9_H

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

class Tr2SamplerStateAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SAMPLER_STATE>
{
public:
	Tr2SamplerStateAL();
	~Tr2SamplerStateAL();

	ALResult Create(
		Tr2RenderContextAL& renderContext,
		const Tr2SamplerDescription& description );
	bool IsValid() const;

	bool operator==( const Tr2SamplerStateAL& ) const;

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

private:
	friend class Tr2RenderContextAL;

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

#endif // TRINITY_PLATFORM==TRINITY_OPENGLES2

#endif // Tr2SamplerStateALDx9_H
