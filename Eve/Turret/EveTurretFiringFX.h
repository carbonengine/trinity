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

// forwards
BLUE_DECLARE( EveStretch );
BLUE_DECLARE_VECTOR( EveStretch );

// --------------------------------------------------------------------------------
// Description:
//   This class holds a turret firing effect. This effect might be made out of
//   different parts like EveStretch, etc.
// SeeAlso:
//   EveStretch
// --------------------------------------------------------------------------------
class EveTurretFiringFX :
	public IInitialize,
	public INotify
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
	
public:
	bool ReadyToFire() const;
	// timing and worldspace positioning
	bool Update( Be::Time time, float deltaT );
	// rendering
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables );
        // bounds
	bool GetBoundingSphere( Vector4& bounds ) const;

	// query: get the number of per-muzzle effects
	unsigned int GetPerMuzzleEffectCount() const;
	// query: get bone id
	unsigned int GetPerMuzzleBoneID( int muzzleID ) const;
	// query: is looping
	bool IsLooping() const;

	// setup this effect: muzzle bone IDs
	void SetMuzzleBoneID( int muzzleID, unsigned int boneID );
	// setup this effect: muzzle position
	void SetMuzzleTransform( int muzzleID, const Matrix* transform );
	// setup this effect: end position
	void SetEndPosition( const Vector3* endPos );
	// set the scale of the destination object
	void SetScaleByRadius( float radius );

	// action: prepare (== "start with delay" ) firing
	void PrepareFiring( float delay, unsigned int muzzleID = 0xffffffff, unsigned int muzzleCount = 0xffffffff );
	// action: stop all firing
	void StopFiring();

	// toggle display of source and dest objects of the stretcher
	void SetDisplayDestObject( bool display ) { m_displayDestObject = display; }
	bool GetDisplayDestObject() const { return m_displayDestObject; }
	void SetDisplaySourceObject( bool display ) { m_displaySourceObject = display; }
	bool GetDisplaySourceObject() const { return m_displaySourceObject; }

private:
	float GetCurveDuration();

	// Maximum curve duration of firing effect
	float m_firingDuration;

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
	PEveStretchVector m_stretch;
};

TYPEDEF_BLUECLASS( EveTurretFiringFX );

#endif // EveTurretFiringFX_H
