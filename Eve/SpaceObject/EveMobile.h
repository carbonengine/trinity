////////////////////////////////////////////////////////////
//
//    Created:   October 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EveMobile_H
#define EveMobile_H

#include "EveSpaceObject2.h"

// forwards
BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( EveTurretSet );
BLUE_DECLARE_VECTOR( EveTurretSet );

// --------------------------------------------------------------------------------
// Description:
//   This class adds functionalty like turrets to the spaceobjects class
// SeeAlso:
//   EveSpaceObject2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveMobile ) :
	public EveSpaceObject2,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	EveMobile( IRoot* lockobj = NULL );
	~EveMobile();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// Overrides of EveSpaceObject2 implementations
	virtual void UpdateSyncronous( EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext );
	virtual void PrepareShaderData( EveUpdateContext& updateContext );
	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	virtual bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );
	virtual void GetLights(Tr2LightManager& lightManager) const;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveShadowCaster - overriding EveSpaceObject2 implementations
	virtual bool GetRenderablesCastingShadow( bool isSelf, const TriFrustumOrtho& frustum, std::vector<ITr2Renderable*>& renderables );

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
    virtual void RenderDebugInfo( Tr2DebugRenderer& renderer );

	/////////////////////////////////////////////////////////////////////////////////////
	// Overrides of animation controller
	virtual bool ExecuteAnimationStateCommand( const EveAnimationCommand& cmd, const std::map<std::string, float>& parameters );

	// re-positions all attached turrets to the corresponding locators
	void RebuildTurretPositions();
	// checks and counts the number of locators and/or granny-bones used to position turrets
	unsigned int GetTurretLocatorCount();
	// Asynch update for turret sets
	virtual void UpdateTurretsAsyncronous( EveUpdateContext& updateContext );

	/////////////////////////////////////////////////////////////////////////////////////
	// activation
	void PlayActivationCurve();

    /////////////////////////////////////////////////////////////////////////////////////
	// clip sphere modification
	void PlayClipSphereFactorCurve();
	void ModifyClipSphereCurve( const std::map<std::string, float>& parameters );

protected:
	/////////////////////////////////////////////////////////////////////////////////////
	// children
	virtual bool DisplayChildren() const;

	/////////////////////////////////////////////////////////////////////////////////////
	// activation
	float m_activationStrenght;
	
	/////////////////////////////////////////////////////////////////////////////////////
	// turrets
	PEveTurretSetVector m_turretSets;
	virtual const Matrix* GetTurretTransform( unsigned int turretSetIndex ) const;
private:
	/////////////////////////////////////////////////////////////////////////////////////
	// activation
	ITriScalarFunctionPtr m_activationStrengthCurve;
	bool m_playActivationCurve;
	float m_activationDelta;

	/////////////////////////////////////////////////////////////////////////////////////
	// dissolve
	float m_clipSphereFactor;
	Vector3 m_clipSphereCenter;

	/////////////////////////////////////////////////////////////////////////////////////
	// turrets
	// ship-internal data on turret locators
	struct TurretSetLocatorInfo
	{
		// how this turretset is attached
		EveSpaceObject2::LocatorType type;
		// all locator indices of this turretset
		std::vector<unsigned int> locatorIndices;
	};
	std::vector<TurretSetLocatorInfo> m_turretSetsLocatorInfo;
	size_t GetTurretLocatorIndex( size_t turretSetIdx, size_t slotIdx ) const;

	// turret locator counting
	struct TurretLocatorCountingInfo
	{
		unsigned int currentCount;
		unsigned int totalCount;
	};
	std::map<std::string, TurretLocatorCountingInfo> m_turretLocatorCountingInfo;
	void ResetTurretLocatorCounter( bool updateTotal );
	bool GetTurretLocatorCountingInfo( const char* name, unsigned int& current, unsigned int& total ) const;
	bool ValidateTurretLocatorName( const char* locatorName, unsigned int& locatorsFoundA, unsigned int& locatorsFoundB ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// clip sphere modification
	bool m_playClipSphereFactorCurve; 
	float m_clipSphereFactorDelta;
	ITriScalarFunctionPtr m_clipSphereFactorCurve;
};

TYPEDEF_BLUECLASS( EveMobile );

#endif // EveMobile_H
