#pragma once
#ifndef EveAnimationSequencer_H
#define EveAnimationSequencer_H

#include "BlueExposure/include/BlueTypes.h"

BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( EveAnimationState );
BLUE_DECLARE_VECTOR( EveAnimationState );

BLUE_CLASS( EveAnimationStateSequencer ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	
	EveAnimationStateSequencer( IRoot* lockobj = NULL );
	~EveAnimationStateSequencer();

	void Clear();
	void Update( Be::Time time );
	void PushState( EveAnimationState* state );
private:
	EveAnimationStatePtr m_currentState;
	PEveAnimationStateVector m_pendingStates;
	EveSpaceObject2Ptr m_owner;

	bool m_update;
};
TYPEDEF_BLUECLASS( EveAnimationStateSequencer );

#endif