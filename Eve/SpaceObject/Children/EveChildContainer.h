////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#pragma once
#ifndef EveChildContainer_H
#define EveChildContainer_H

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"

BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );

BLUE_CLASS( EveChildContainer ) :
	public IEveSpaceObjectChild,
	public EveChildTransform
{
public:
	EXPOSE_TO_BLUE();

	EveChildContainer( IRoot* lockobj = NULL );
	~EveChildContainer();

	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	
	void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent );
	void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, Matrix& parentTransform );
	void GetLocalToWorldTransform( Matrix& transform ) const;

	void Transform( const Vector3* scale, const Quaternion* rotation, const Vector3* translation );

	void PlayCurveSet( const std::string& name );
	void StopCurveSet( const std::string& name );
	float GetCurveSetDuration( const std::string& name ) const;

private:
	BlueSharedString m_name;
	bool m_display;
	PIEveSpaceObjectChildVector m_objects;
	PTriCurveSetVector m_curveSets;
};

TYPEDEF_BLUECLASS( EveChildContainer );

#endif