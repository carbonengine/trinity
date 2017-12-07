#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2SamplerStateALGLES2.h"
#include "Tr2HalHelperStructures.h"


#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D GL_TEXTURE_3D_OES
#define GL_TEXTURE_WRAP_R GL_TEXTURE_WRAP_R_OES
#endif

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{

	Tr2SamplerStateAL::Tr2SamplerStateAL()
		: m_isValid( false )
	{
		m_stateData.m_hash = 0;
		m_stateData.m_magFilter = GL_LINEAR;
		m_stateData.m_minFilter = GL_LINEAR;
		m_stateData.m_minFilterNoMips = GL_LINEAR;
		m_stateData.m_wrapT = GL_REPEAT;
		m_stateData.m_wrapS = GL_REPEAT;
		m_stateData.m_wrapR = GL_REPEAT;
		m_stateData.m_anisotropy = 0;
#if !defined(TRINITY_AL_MOBILE)
		m_stateData.m_minLod = 0.f;
		m_stateData.m_maxLod = std::numeric_limits<float>::max();
#endif
		m_stateData.m_hash = CcpHashFNV1( &m_stateData, sizeof( m_stateData ) );
	}

	ALResult Tr2SamplerStateAL::Create( const Tr2SamplerDescription& description, Tr2RenderContextAL& /*renderContext*/ )
	{
		CR_RETURN_HR( CreateStateData( description, m_stateData ) );
		m_isValid = true;
		return S_OK;
	}

	void Tr2SamplerStateAL::Destroy()
	{
		m_isValid = false;
	}

	bool Tr2SamplerStateAL::IsValid() const
	{
		return m_isValid;
	}

	Tr2ALMemoryType Tr2SamplerStateAL::GetMemoryClass() const 
	{ 
		return AL_MEMORY_MANAGED; 
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
			stateData.m_minFilterNoMips = GL_NEAREST;
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
			stateData.m_minFilterNoMips = GL_LINEAR;
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
#if !defined(TRINITY_AL_MOBILE)
		stateData.m_minLod = description.m_minLOD;
		stateData.m_maxLod = description.m_maxLOD;
#endif
		stateData.m_hash = CcpHashFNV1( &stateData, sizeof( stateData ) );
		return S_OK;
	}

	ALResult Tr2SamplerStateAL::Apply( GLenum textureType, bool hasMipLevels, const StateData& stateData )
	{
		CR_GL( glTexParameteri( textureType, GL_TEXTURE_MIN_FILTER, hasMipLevels ? stateData.m_minFilter : stateData.m_minFilterNoMips ) );
		CR_GL( glTexParameteri( textureType, GL_TEXTURE_MAG_FILTER, stateData.m_magFilter ) );
		CR_GL( glTexParameteri( textureType, GL_TEXTURE_WRAP_S, textureType == GL_TEXTURE_CUBE_MAP ? GL_CLAMP_TO_EDGE : stateData.m_wrapT ) );
		CR_GL( glTexParameteri( textureType, GL_TEXTURE_WRAP_T, textureType == GL_TEXTURE_CUBE_MAP ? GL_CLAMP_TO_EDGE : stateData.m_wrapS ) );
		if( textureType == GL_TEXTURE_3D )
		{
			CR_GL( glTexParameteri( textureType, GL_TEXTURE_WRAP_R, stateData.m_wrapR ) );
		}
		// setting max anisotropy might fail if ext. is not there, but it is
		// ok, since then we'll have nice fallback to linear/nearest
		GL_IGNORE_ERROR( glTexParameterf( textureType, GL_TEXTURE_MAX_ANISOTROPY_EXT, GLfloat( std::max( stateData.m_anisotropy, 1 ) ) ) );
#if !defined(TRINITY_AL_MOBILE)
		CR_GL( glTexParameterf( textureType, GL_TEXTURE_MIN_LOD, stateData.m_minLod ) );
		CR_GL( glTexParameterf( textureType, GL_TEXTURE_MAX_LOD, stateData.m_maxLod ) );
#endif
		return S_OK;
	}

}
#endif