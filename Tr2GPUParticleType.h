
#pragma once
#ifndef Tr2GPUParticleType_h
#define Tr2GPUParticleType_h

#include "Tr2GPUParticlePool.h"

BLUE_DECLARE( Tr2GPUParticlePoolManager );

//Essentially a blue exposure of the ParticleBehaviour POD struct,
//for editing and saving in .red files.
class Tr2GPUParticleType: public INotify 
{
public:
	EXPOSE_TO_BLUE();
	Tr2GPUParticleType( IRoot *lockObj = nullptr );
	bool OnModified( Be::Var* value	);
	bool CheckForUpdate();

	Tr2GPUParticlePool *GetPool() { return m_pool; }

	Tr2GPUParticleRenderMode m_renderMode;

	int GetBehaviourIndex();

	const std::string &GetBehaviourName() { return m_name; }
	void ApplyPool( Tr2GPUParticlePoolManager* manager );
private:
	std::string m_name, m_previousBehaviourName;
	std::string m_deathTransition, m_collideTransition;
	Tr2ParticleBehaviour m_behaviour;

	std::string m_previousTexturePath;
	std::string m_previousGradientPath;
	static std::string s_defaultTexturePath, s_defaultGradientPath;
	
	Tr2GPUParticlePoolPtr m_pool;
	bool m_needsPush;
	bool PushToPool();

	int m_behaviourIndex;
};

TYPEDEF_BLUECLASS( Tr2GPUParticleType );
BLUE_DECLARE( Tr2GPUParticleType );
BLUE_DECLARE_VECTOR( Tr2GPUParticleType );


#endif//Tr2GPUParticleType_h