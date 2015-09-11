////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#pragma once
#ifndef EveChildContainer_H
#define EveChildContainer_H

#include "IEveSpaceObjectChild.h"

BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );

BLUE_CLASS( EveChildContainer ) :
	public IEveSpaceObjectChild
{
public:
	EXPOSE_TO_BLUE();

	EveChildContainer( IRoot* lockobj = NULL );
	~EveChildContainer();

	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	
	void UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );
	
	void PlayCurveSet( const std::string& name );
	void StopCurveSet( const std::string& name );
	float GetCurveSetDuration( const std::string& name ) const;

private:
	BlueSharedString m_name;
	PIEveSpaceObjectChildVector m_objects;
	PTriCurveSetVector m_curveSets;
};

TYPEDEF_BLUECLASS( EveChildContainer );

#endif