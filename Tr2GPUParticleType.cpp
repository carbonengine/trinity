#include <StdAfx.h>

#include "Tr2GPUParticleType.h"
#include "Tr2GPUParticlePoolManager.h"

//==========================
//Tr2GPUParticleType
// Basically wraps a particle behaviour

std::string Tr2GPUParticleType::s_defaultTexturePath = "res:/Texture/Global/GPUParticles/white_flare.dds";
std::string Tr2GPUParticleType::s_defaultGradientPath = "res:/Texture/Global/GPUParticles/white.dds";

Tr2GPUParticleType::Tr2GPUParticleType( IRoot *lockobj ) :
	m_needsPush(false), 
	m_renderMode( GPUPRM_Additive ),
	m_behaviourIndex(0)
{

	m_behaviour.lifespan = 1.0;
	m_behaviour.colour[0] = Color(1,1,1,1);
	m_behaviour.colour[1] = Color(1,1,1,1);
	m_behaviour.colour[2] = Color(1,1,1,1);
	m_behaviour.colour[3] = Color(0,0,0,0);
	m_behaviour.size = Vector3(100, 100, 100);

	m_behaviour.texturePath = s_defaultTexturePath;
	m_behaviour.gradientPath = s_defaultGradientPath;
}

//helper function, given a particle pool it pushes a texture
// and possibly a gradient path onto that pool's texture atlas(es)
bool UpdateTextureAtlas( Tr2GPUParticlePool *pool, 
	const std::string &oldPath, const std::string &path, Vector4 &window,
	const std::string &oldGrad, const std::string &grad, Vector4 &gradWindow ) 
{
	if( pool ) 
	{
		bool result = true;

		if( !path.empty() ) 
		{
			result = pool->AddTexture( path, window );
			pool->ReleaseTexture( oldPath );
		}

		if( !grad.empty() ) 
		{
			result = result && pool->AddGradient( grad, gradWindow );
			pool->ReleaseGradient( oldGrad );
		}

		return result;
	}
	return false;
}

// -------------------------------------------------------------
// Description:
//  Fetches a pool which has an appropriate rendering mode, and stores 
//  it in the member m_pool.
// -------------------------------------------------------------
void Tr2GPUParticleType::ApplyPool( Tr2GPUParticlePoolManager* manager ) 
{
	if( manager && manager->m_particles.size() )
	{
		for( auto it = manager->m_particles.begin();
			it != manager->m_particles.end();
			++it )
		{
			if( (*it)->GetRenderMode() == m_renderMode )
			{
				m_pool = *it;
				return;
			}
		}
	}

	m_pool = nullptr;
}

// -------------------------------------------------------------
// Description:
//  Should be called by Jessica etc. when the properties of this object
//  are modified, indicating we need to update the particle pool to
//  reflect the new properties.
// -------------------------------------------------------------
bool Tr2GPUParticleType::OnModified( Be::Var* value	)
{
	if( m_name != m_previousBehaviourName )
	{
		m_behaviourIndex = 0;
		GetBehaviourIndex();
		m_previousBehaviourName = m_name;		
	}

	if( IsMatch( value, m_renderMode ) )
	{
		//need to change pool when we swap rendering mode
		m_needsPush = true;
	}

	m_needsPush = !m_pool->HasBehaviour( m_name );
	return true;
}

// -------------------------------------------------------------
// Description:
//  Pushes the new behaviour to the particle pool, where it will
//  find its way to the behaviour texture on next update.
// -------------------------------------------------------------
bool Tr2GPUParticleType::PushToPool()
{
	if( m_pool && m_name.length() > 0 )
	{
		m_behaviour.collideTransition = m_pool->GetBehaviourIndex( m_collideTransition.c_str() ) / float(m_pool->GetBehaviourMax());
		m_behaviour.deathTransition = m_pool->GetBehaviourIndex( m_deathTransition.c_str() ) / float(m_pool->GetBehaviourMax());

		if( m_behaviour.texturePath.empty() )
		{
			m_behaviour.texturePath = s_defaultTexturePath;
		}
		if( m_behaviour.gradientPath.empty() )
		{
			m_behaviour.gradientPath = s_defaultGradientPath;
		}
		
		m_needsPush = !UpdateTextureAtlas( m_pool, 
			m_previousTexturePath, m_behaviour.texturePath, m_behaviour.textureWindow,
			m_previousGradientPath, m_behaviour.gradientPath, m_behaviour.gradientWindow );

		CCP_LOG( "Pushed behaviour %s to pool", m_name.c_str() );
		
		if( m_behaviourIndex <= 0 )
		{
			GetBehaviourIndex();
		}

		return m_pool->SetBehaviour( m_name.c_str(), m_behaviour );
	}
	return false;
}

// -------------------------------------------------------------
// Description:
//  Calls PushToPool if properties have changed.
// -------------------------------------------------------------
bool Tr2GPUParticleType::CheckForUpdate()
{
	if( !m_pool ) return false;
	if( m_needsPush || !m_pool->HasBehaviour(m_name) ) return PushToPool();
	return true;
}


// -------------------------------------------------------------
// Description:
//  Helper function that will return either a cached behaviour
//  index, or find an appropriate one. Index zero is reserved for
//  the default, non-rendered behaviour and can be used if the
//  pool is null or the behaviour texture is full.
// -------------------------------------------------------------
int Tr2GPUParticleType::GetBehaviourIndex()
{
	if( m_behaviourIndex > 0 ) return m_behaviourIndex;

	if( !m_pool ) return 0;
	
	m_behaviourIndex = m_pool->GetBehaviourIndex( m_name.c_str() );
	return m_behaviourIndex;
}
