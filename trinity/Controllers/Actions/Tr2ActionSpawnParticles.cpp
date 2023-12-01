////////////////////////////////////////////////////////////
//
//    Created:   September 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionSpawnParticles.h"
#include "Particle/Tr2DynamicEmitter.h"


Tr2ActionSpawnParticles::Tr2ActionSpawnParticles( IRoot* )
	:m_rate( 1.f )
{
}

void Tr2ActionSpawnParticles::Start( Tr2Controller& )
{
	if( !m_emitter )
	{
		return;
	}
	m_emitter->SpawnParticles( ITr2GenericEmitter::UpdateArguments(), nullptr, nullptr, m_rate );
}
