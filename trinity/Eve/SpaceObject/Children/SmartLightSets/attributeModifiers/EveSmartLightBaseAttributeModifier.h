// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "IEveSmartLightGroupAttributeModifier.h"
#include "Lights/Tr2PointLight.h"
#include <Eve/EveUpdateContext.h>
#include <Eve/SpaceObject/Children/IEveSpaceObjectChild.h>
#include "Curves/Tr2CurveScalar.h"

BLUE_DECLARE( Tr2CurveScalar );

BLUE_CLASS( EveSmartLightBaseAttributeModifier ) :
	public IInitialize,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightBaseAttributeModifier( IRoot* lockobj = nullptr );

	void UpdateActivationStrength( float parentActivationMultiplier, float deltaTime );
	virtual void ResetPlayTime( bool active );

	// IInitialize
	bool Initialize() override;

	// INotify
	bool OnModified( Be::Var * val );

	enum LifeTimeFormulas
	{
		PER_INSTANCE_LIFETIME = 0,
		PER_MODIFIER_PLAYTIME = 1,
		STATIC = 2,
	};

protected:
	float GetActivationStrength( const PlacementDataWithIdentifier& placement );
	void MapActivationValue();
	virtual void ResetChildren( bool parentActive ) {};

	std::string m_name;
	Tr2CurveScalarPtr m_activationOverLifetime;
	LifeTimeFormulas m_lifeTimeFormula;

	bool m_active;
	bool m_startsActive;
	bool m_isChangingActivation;
	bool m_restartPlayTimeWhenInactive;
	float m_perInstanceOffset;
	float m_finalActivationStrength;
	float m_activationStrength;
	float m_crossFadeDuration;
	float m_crossFadeIntensity;
	float m_activationValue;
	float m_activationValuePreMapped;
	float m_playTime;
	float m_delayedActivation;
	float m_lastActivationTimeStamp;
};

TYPEDEF_BLUECLASS( EveSmartLightBaseAttributeModifier );
