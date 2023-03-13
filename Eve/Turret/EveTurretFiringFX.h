////////////////////////////////////////////////////////////
//
//    Created:   July 2011
//    Copyright: CCP 2011
//
#pragma once
#ifndef EveTurretFiringFX_H
#define EveTurretFiringFX_H

#include "ITr2Renderable.h"
#include "TriFrustum.h"
#include "Controllers/ITr2ControllerOwner.h"
#include "Eve/SpaceObject/Children/IEveEffectChildrenOwner.h"
#include "Include/ITriTargetable.h"
#include "Tr2DebugRenderer.h"

// forwards
BLUE_DECLARE_INTERFACE( IEveFiringEffectElement );
BLUE_DECLARE_IVECTOR( IEveFiringEffectElement );
BLUE_DECLARE_INTERFACE( IEveEffectChildrenOwner );
BLUE_DECLARE_INTERFACE( IEveSpaceObjectChild );
BLUE_DECLARE_IVECTOR( IEveSpaceObjectChild );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE( TriObserverLocal );
BLUE_DECLARE( Tr2QuadRenderer );
class EveUpdateContext;
class Tr2LightManager;

// --------------------------------------------------------------------------------
// Description:
//   This class holds a turret firing effect. This effect might be made out of
//   different parts like EveStretch, etc.
// SeeAlso:
//   EveStretch
// --------------------------------------------------------------------------------
class EveTurretFiringFX :
	public IInitialize,
	public INotify,
	public ITr2ControllerOwner,
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Unlock;
	using IInitialize::Lock;

	EveTurretFiringFX(IRoot* lockobj = NULL);
	~EveTurretFiringFX();
	
	void CleanUp();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();
	
	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	// max muzzle effects
	enum MaxMuzzleCount
	{
		MUZZLECOUNT_MAX = 12
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

public:
	bool ReadyToFire() const;
	// timing and worldspace positioning
	bool UpdateSynchronous( EveUpdateContext& updateContext );
	bool UpdateAsynchronous( EveUpdateContext& updateContext );

	// rendering
	void UpdateVisibility( const TriFrustum& frustum );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );

	// query: get the number of per-muzzle effects
	unsigned int GetPerMuzzleEffectCount() const;
	// query: get bone id
	unsigned int GetPerMuzzleBoneID( int muzzleID ) const;
	// query: is looping
	bool IsLooping() const;
	// query: get start position
	bool GetStartPosition( Vector3& pos ) const;
	// query: get effect length
	float GetFiringDuration() const;
	// query: get effect peak time
	float GetFiringPeakTime() const;
	// get bone-to-be-attached to name
	const char* GetFiringBoneName() const;

	// setup this effect: muzzle bone IDs
	void SetMuzzleBoneID( int muzzleID, unsigned int boneID );
	// setup this effect: muzzle position
	void SetMuzzleTransform( int muzzleID, const Matrix* transform );
	// setup this effect: end position
	void SetEndPosition( const Vector3* endPos );
	// set the scale of the destination object
	void SetScaleByRadius( float radius );
	// set the impact configuration of the target being hit (e.g. shield, armor or hull).
	void SetImpactConfiguration( ITriTargetable::ImpactConfiguration impactConfiguration );

	// reset the move objets when looping
	void PrepareFiringEffectMoveObjects();
	// action: prepare (== "start with delay" ) firing
	void PrepareFiring( float delay, unsigned int muzzleID = 0xffffffff, unsigned int muzzleCount = 0xffffffff );
	// action: stop all firing
	void StopFiring();

	// toggle display of source and dest objects of the stretcher
	void SetDisplayDestObject( bool display ) { m_displayDestObject = display; }
	bool GetDisplayDestObject() const { return m_displayDestObject; }
	void SetDisplaySourceObject( bool display ) { m_displaySourceObject = display; }
	bool GetDisplaySourceObject() const { return m_displaySourceObject; }

	void GetLights( Tr2LightManager& lightManager ) const;

	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );

	// ITr2ControllerOwner
	void SetControllerVariable( const char* name, float value );
	void HandleControllerEvent( const char* name );
	void StartControllers();

private:
	float GetCurveDuration();

	// Maximum curve duration of firing effect
	float m_firingDuration;
	// peak time of effect
	float m_firingPeakTime;
	// for our very special case...
	float m_firingDurationOverride;

	// per-muzzle effects
	void StartMuzzleEffect( int muzzleID );

	// Used for scaling the target object of the stretch object
	bool m_scaleEffectTarget;
	float m_minRadius;
	float m_maxRadius;
	float m_minScale;
	float m_maxScale;

	// visible
	bool m_display;
	bool m_displaySourceObject;
	bool m_displayDestObject;
	// name
	std::string m_name;
	// all firing effects share a destination point
	Vector3 m_endPosition;
	// use end position or muzzle transform?
	bool m_useMuzzleTransform;
	// do we have something to show? (= is it firing?)
	bool m_isFiring;
	// some turret effects (like miners or salvagers) loop the firing effect endlessly
	bool m_isLoopFiring;
	// to-be-attached to bone name
	BlueSharedString m_boneName;

    // firing effect data (is of fixed length, so there is a max muzzle count per turret!)
	struct PerMuzzleData
	{
		// state of the effect
		bool started, readyToStart;
		// specifies the source of the firing stretch effect
		unsigned int muzzlePositionBoneID;
		Matrix muzzleTransform;
		// handle time delay to firing to be in sync with animation
		float currentStartDelay;
		float constantDelay;
		// keep track of effect progress
		float elapsedTime;
	};
	PerMuzzleData m_perMuzzleData[MUZZLECOUNT_MAX];

	// vector of stretch effect
	PIEveFiringEffectElementVector m_stretch;
	TriCurveSetPtr m_startCurveSet;
	TriCurveSetPtr m_stopCurveSet;
	TriObserverLocalPtr m_sourceObserver;
	TriObserverLocalPtr m_destinationObserver;

	// The configuration of impact overlay on the target, e.g. whether the impact is hitting the target's shield, armor or hull
	ITriTargetable::ImpactConfiguration m_impactConfiguration;
};

TYPEDEF_BLUECLASS( EveTurretFiringFX );

#endif // EveTurretFiringFX_H
