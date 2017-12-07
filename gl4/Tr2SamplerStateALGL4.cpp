#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "Tr2SamplerStateALGL4.h"
#include "Tr2HalHelperStructures.h"

#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D GL_TEXTURE_3D_OES
#define GL_TEXTURE_WRAP_R GL_TEXTURE_WRAP_R_OES
#endif

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{

	Tr2SamplerStateAL::Tr2SamplerStateAL()
		: m_isValid( false ),
		m_sampler( 0 ),
		m_clObject( nullptr )
	{
		m_stateData.m_hash = 0;
		m_stateData.m_magFilter = GL_LINEAR;
		m_stateData.m_minFilter = GL_LINEAR;
		m_stateData.m_wrapT = GL_REPEAT;
		m_stateData.m_wrapS = GL_REPEAT;
		m_stateData.m_wrapR = GL_REPEAT;
		m_stateData.m_anisotropy = 0;
		m_stateData.m_minLod = 0.f;
		m_stateData.m_maxLod = std::numeric_limits<float>::max();
		m_stateData.m_hash = CcpHashFNV1( &m_stateData, sizeof( m_stateData ) );
	}

	Tr2SamplerStateAL::~Tr2SamplerStateAL()
	{
		if( m_clObject )
		{
			clReleaseSampler( m_clObject );
		}
	}

	ALResult Tr2SamplerStateAL::Create( const Tr2SamplerDescription& description, Tr2RenderContextAL& /*renderContext*/ )
	{
		if( m_sampler )
		{
			glDeleteSamplers( 1, &m_sampler );
			m_sampler = 0;
		}
		glGenSamplers( 1, &m_sampler );
		if( !m_sampler )
		{
			return E_FAIL;
		}
		CR_RETURN_HR( CreateStateData( description, m_stateData ) );

		CR_GL( glSamplerParameteri( m_sampler, GL_TEXTURE_MIN_FILTER, m_stateData.m_minFilter ) );
		CR_GL( glSamplerParameteri( m_sampler, GL_TEXTURE_MAG_FILTER, m_stateData.m_magFilter ) );
		CR_GL( glSamplerParameteri( m_sampler, GL_TEXTURE_WRAP_S, m_stateData.m_wrapT ) );
		CR_GL( glSamplerParameteri( m_sampler, GL_TEXTURE_WRAP_T, m_stateData.m_wrapS ) );
		CR_GL( glSamplerParameteri( m_sampler, GL_TEXTURE_WRAP_R, m_stateData.m_wrapR ) );

		CR_GL( glSamplerParameterf( m_sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, GLfloat( std::max( m_stateData.m_anisotropy, 1 ) ) ) );
		CR_GL( glSamplerParameterf( m_sampler, GL_TEXTURE_MIN_LOD, m_stateData.m_minLod ) );
		CR_GL( glSamplerParameterf( m_sampler, GL_TEXTURE_MAX_LOD, m_stateData.m_maxLod ) );
		CR_GL( glSamplerParameteri( m_sampler, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT ) );

		m_isValid = true;
		return S_OK;
	}

	void Tr2SamplerStateAL::Destroy()
	{
		if( m_clObject )
		{
			clReleaseSampler( m_clObject );
			m_clObject = nullptr;
		}
		m_isValid = false;
	}

	bool Tr2SamplerStateAL::IsValid() const
	{
		return m_isValid;
	}

	ALResult Tr2SamplerStateAL::CreateStateData( const Tr2SamplerDescription& description, StateData& stateData )
	{
		extern uint32_t g_forceAnisotropy;

		switch( description.m_minFilter )
		{
		case TF_POINT:
			switch( description.m_mipFilter )
			{
			case TF_NONE:
				stateData.m_minFilter = GL_NEAREST;
				break;
			case TF_POINT:
				stateData.m_minFilter = GL_NEAREST_MIPMAP_NEAREST;
				break;
			default:
				stateData.m_minFilter = GL_NEAREST_MIPMAP_LINEAR;
			}
			break;
		default:
			switch( description.m_mipFilter )
			{
			case TF_NONE:
				stateData.m_minFilter = GL_LINEAR;
				break;
			case TF_POINT:
				stateData.m_minFilter = GL_LINEAR_MIPMAP_NEAREST;
				break;
			default:
				stateData.m_minFilter = GL_LINEAR_MIPMAP_LINEAR;
			}
		}
		if( description.m_magFilter == TF_POINT )
		{
			stateData.m_magFilter = GL_NEAREST;
		}
		else
		{
			stateData.m_magFilter = GL_LINEAR;
		}

		static const int wrapModes[] = {
			0,					// ---
			GL_REPEAT,          // TA_WRAP
			GL_MIRRORED_REPEAT, // TA_MIRROR
			GL_CLAMP_TO_EDGE,   // TA_CLAMP
			GL_CLAMP_TO_EDGE,   // TA_BORDER
			GL_CLAMP_TO_EDGE,   // TA_MIRROR_ONCE
		};

		stateData.m_wrapT = wrapModes[description.m_addressU];
		stateData.m_wrapS = wrapModes[description.m_addressV];
		stateData.m_wrapR = wrapModes[description.m_addressW];

		if( description.m_minFilter == TF_ANISOTROPIC || description.m_magFilter == TF_ANISOTROPIC || description.m_mipFilter == TF_ANISOTROPIC )
		{
			stateData.m_anisotropy = g_forceAnisotropy ? g_forceAnisotropy : std::max( description.m_maxAnisotropy, 1u );
		}
		else
		{
			stateData.m_anisotropy = 1;
		}
		stateData.m_minLod = description.m_minLOD;
		stateData.m_maxLod = description.m_maxLOD;
		stateData.m_hash = CcpHashFNV1( &stateData, sizeof( stateData ) );
		return S_OK;
	}

	Tr2ALMemoryType Tr2SamplerStateAL::GetMemoryClass() const 
	{ 
		return AL_MEMORY_MANAGED; 
	}

}

#endif