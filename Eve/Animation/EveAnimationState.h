#pragma once
#ifndef EveAnimationState_H
#define EveAnimationState_H

#include "BlueExposure/include/BlueTypes.h"
#include "EveAnimationData.h"

BLUE_DECLARE( EveSpaceObject2 );

enum EveAnimationStage
{
	EVE_ANISTAGE_ENTER,
	EVE_ANISTAGE_MAIN,
	EVE_ANISTAGE_EXIT,
	EVE_ANISTAGE_TRANSITION,
	EVE_ANISTAGE_INVALID
};

enum EveAnimationStateProgress {
	EVE_ANIM_INACTIVE = 32,
	EVE_ANIM_ENTERING = 1,
	EVE_ANIM_RUNNING = 2,
	EVE_ANIM_FINALIZE_RUN = 64,
	EVE_ANIM_EXITING = 4,
	EVE_ANIM_TRANSITIONING = 8,
	EVE_ANIM_DONE = 16
};

BLUE_CLASS( EveAnimationState ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	
	EveAnimationState( IRoot* lockobj = NULL );
	~EveAnimationState();
	
	void TransitionTo( EveAnimationStatePtr state, EveSpaceObject2Ptr owner );
	void TransitionFrom( EveAnimationStatePtr state, EveSpaceObject2Ptr owner );
	void Update( Be::Time time, EveSpaceObject2Ptr owner );
	
	EveAnimationStateProgress GetProgress() const { return m_progress; }
	const std::string& GetName() const { return m_name; }
	const std::string& GetTranstionStateName() const { return m_transitionName; }
private:
	std::string m_name;

	// Holds the id of a state that we've been requested to transition into or empty string
	std::string m_transitionName;
	bool m_transitionPending;
	bool m_pendingCommands;

	EveAnimationSequencePtr m_enterSequence;
	EveAnimationSequencePtr m_mainSequence;
	EveAnimationSequencePtr m_exitSequence;
	PEveTransitionSequenceVector m_transitions;

	EveAnimationStateProgress m_progress;
	EveAnimationStage m_currentSequence;

	float m_startTime;
	float m_animationDuration;
	float m_secondsRemaining;
	float m_pendingCommandDelay;

	void DoAnimationSequenceCurves( EveSpaceObject2Ptr owner, EveAnimationSequencePtr sequence );
	void DoAnimationSequenceCommands( EveSpaceObject2Ptr owner, EveAnimationSequencePtr sequence );
	void DoAnimationSequence( EveSpaceObject2Ptr owner, EveAnimationSequencePtr sequence );
	void UpdateSequenceDuration( EveSpaceObject2Ptr owner, const EveAnimationSequencePtr sequence );
	bool HasTransition( std::string stateName );
	void DoTransitionSequence( std::string stateName );
	void DoEnterSequence();
	void DoMainSequence();
	void DoExitSequence();

	void PlaySequence( EveSpaceObject2Ptr owner, EveAnimationStage sequence );
	void StopCurrentSequence( EveSpaceObject2Ptr owner, Be::Time time );
	void Cleanup();
	
	EveTransitionSequencePtr GetTransition( std::string stateName );
	EveAnimationSequencePtr GetAnimationSequence( EveAnimationStage stage );
};
TYPEDEF_BLUECLASS( EveAnimationState );
BLUE_DECLARE_VECTOR( EveAnimationState );


BLUE_CLASS( EveAnimationStateContainer ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	
	EveAnimationStateContainer( IRoot* lockobj = NULL ) : PARENTLOCK( m_states ) {}
	~EveAnimationStateContainer() {}

	PEveAnimationStateVector m_states;
	BlueSharedString m_defaultAnimation;
};
TYPEDEF_BLUECLASS( EveAnimationStateContainer );
#endif