#pragma once
#ifndef EveAnimationState_H
#define EveAnimationState_H

#include "BlueExposure/include/BlueTypes.h"
#include "EveAnimationData.h"
#include "Eve/SpaceObject/Attachments/EveMeshOverlayEffect.h"

BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( EveAnimationStateMachine );
BLUE_DECLARE( EveMeshOverlayEffect );

enum EveAnimationStateStartCommand {
	EVE_ANIM_START_DEFAULT,
	EVE_ANIM_START_INIT,
	EVE_ANIM_START_TRANSITION
};

struct EveAnimationStateTransition
{
	BlueSharedString name;
	BlueSharedString transitionName;
};
BLUE_DECLARE_STRUCTURE_LIST( EveAnimationStateTransition );

BLUE_CLASS( EveAnimationState ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	
	EveAnimationState( IRoot* lockobj = NULL );
	~EveAnimationState();
	
	void Start( EveAnimationStateMachine* sm, EveSpaceObject2* so, EveAnimationStateStartCommand mode=EVE_ANIM_START_DEFAULT );
	void Stop( EveAnimationStateMachine* sm, EveSpaceObject2* owner );
	void Update( Be::Time time, EveSpaceObject2* owner );
	
	void SetParameter( const std::string& parameterName, float parameterValue );

	EveAnimationStateProgress GetProgress() const { return m_progress; }
	const std::string& GetName() const { return m_name; }
	const char* GetTransition( const std::string& stateName ) const;

private:
	std::string m_name;
	std::string m_overlayPath;
	
	bool m_doInitialization;

	EveAnimationPtr m_animation;
	PEveAnimationCurveVector m_curves;
	PEveAnimationCommandVector m_commands;
	EveMeshOverlayEffectPtr m_overlay;
	
	PEveAnimationCurveVector m_initCurves;
	PEveAnimationCommandVector m_initCommands;

	PEveAnimationStateTransitionStructureList m_transitions;

	EveAnimationStateProgress m_progress;

	float m_startTime;
	float m_animationDuration;
	float m_secondsRemaining;

	std::map<std::string, float> m_parameters;

	void PlayCurves( EveSpaceObject2* owner );
	void ExecuteCommands( EveSpaceObject2* owner );
	void PlayAnimation( EveAnimationStateMachine* sm, EveSpaceObject2* so );
	void EndAnimation( EveAnimationStateMachine* sm, EveSpaceObject2* owner );
	void UpdateDuration( EveAnimationStateMachine* sm, EveSpaceObject2* so );

	void LoadOverlayEffect( EveSpaceObject2* owner );
	void Cleanup( EveSpaceObject2* owner, Be::Time time );
};
TYPEDEF_BLUECLASS( EveAnimationState );
BLUE_DECLARE_VECTOR( EveAnimationState );
TYPEDEF_BLUECLASS( EveMeshOverlayEffect );
BLUE_DECLARE_VECTOR( EveMeshOverlayEffect );

#endif