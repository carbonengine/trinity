#pragma once
#ifndef EveAnimationSequencer_H
#define EveAnimationSequencer_H

#include "BlueExposure/include/BlueTypes.h"
#include "EveAnimationData.h"

BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( EveAnimationState );
BLUE_DECLARE_VECTOR( EveAnimationState );

BLUE_CLASS( EveAnimationStateMachine ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	
	EveAnimationStateMachine( IRoot* lockobj = NULL );
	~EveAnimationStateMachine();

	void Clear();
	void Update( EveSpaceObject2* owner, Be::Time time );
	void GoToState( EveSpaceObject2* owner, const std::string& name );
	void ForceState( EveSpaceObject2* owner, const std::string& name );
	const char* GetEndStateName( );
	void SetStateParameter( const std::string& stateName, const std::string& parameterName, float parameterValue );
	void Rebuild();

	const char* GetDefaultAnimation() const { return m_defaultAnimation.c_str(); }
	void SetOwner( EveSpaceObject2* owner );

	bool HasTrackMask() const { return !m_trackMask.empty(); };
	const char* GetTrackMask() const { return m_trackMask.c_str(); };

private:
	bool m_update;
	bool m_isTransitioning;
	bool m_autoPlayDefault;

	EveAnimationStatePtr m_currentState;
	PEveAnimationStateVector m_pendingStates;

	PEveAnimationStateVector m_states;
	PEveAnimationStateVector m_transitions;
	BlueSharedString m_defaultAnimation;
	BlueSharedString m_defaultState;
	BlueSharedString m_trackMask;
	BlueSharedString m_name;
	
	EveAnimationStatePtr GetAnimationState( const std::string& name, PEveAnimationStateVector& states );
	bool CheckCompletionAndChangeStates( EveSpaceObject2* owner );
};
TYPEDEF_BLUECLASS( EveAnimationStateMachine );
BLUE_DECLARE_VECTOR( EveAnimationStateMachine );


BLUE_CLASS( EveAnimationSequencer ) :
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();
	
	EveAnimationSequencer( IRoot* lockobj = NULL );
	~EveAnimationSequencer();
	
	void SetOwner( EveSpaceObject2* owner );
	void Update( Be::Time time );
	void GoToState( const std::string& name );
	void ForceState( const std::string& name );
	void SetStateParameter( const std::string& stateName, const std::string& parameterName, float parameterValue );
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );

private:
	PEveAnimationStateMachineVector m_stateMachines;
	BlueWeakRef<EveSpaceObject2> m_owner;
};
TYPEDEF_BLUECLASS( EveAnimationSequencer );
#endif