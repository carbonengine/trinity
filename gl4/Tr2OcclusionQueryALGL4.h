#pragma once
#ifndef Tr2OcclusionQueryALDx9_H
#define Tr2OcclusionQueryALDx9_H

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

class Tr2OcclusionQueryAL : 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_OCCLUSION_QUERY>
{
public:
	enum WaitMode
	{
		WAIT,
		DO_NOT_WAIT,
	};

	Tr2OcclusionQueryAL();
	~Tr2OcclusionQueryAL();

	ALResult Create( Tr2RenderContextAL& renderContext );
	bool IsValid() const;
	void Destroy();

	ALResult Begin( Tr2RenderContextAL& renderContext );
	ALResult End( Tr2RenderContextAL& renderContext );
	ALResult GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, WaitMode waitMode = DO_NOT_WAIT );

	bool operator==( const Tr2OcclusionQueryAL& other ) const { return m_query == other.m_query; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }
private:
	Tr2OcclusionQueryAL( const Tr2OcclusionQueryAL& ) /* = delete */;
	Tr2OcclusionQueryAL& operator=( const Tr2OcclusionQueryAL& ) /* = delete */;

	GLuint m_query;
};

#endif

#endif // Tr2OcclusionQueryALDx9_H