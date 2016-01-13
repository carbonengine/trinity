////////////////////////////////////////////////////////////
//
//    Created:   2015
//    Copyright: CCP 2015
//
#pragma once
#ifndef EveLineContainer_H
#define EveLineContainer_H

#include "Eve/IEveSpaceObject2.h"

BLUE_DECLARE( EveConnector );
BLUE_DECLARE_VECTOR( EveConnector );
BLUE_DECLARE( EveCurveLineSet );


BLUE_CLASS( EveLineContainer ) :
	public IEveSpaceObject2
{
public:
	EXPOSE_TO_BLUE();
	using IEveSpaceObject2::Lock;
	using IEveSpaceObject2::Unlock;

	EveLineContainer( IRoot* lockobj = NULL );
	~EveLineContainer();

	void Update( EveUpdateContext& context );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	void UpdateSyncronous( EveUpdateContext& updateContext ) {}
	void UpdateAsyncronous( EveUpdateContext& updateContext ) { Update( updateContext ); }
	void RenderDebugInfo( Tr2RenderContext& renderContext ) {}
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;

	// This version of the function should perform an update on the model / ball position
	void GetModelCenterWorldPosition( Vector3 &position, Be::Time t );

	// This version of the function should not update the object
	void GetCurrentModelCenterWorldPosition( Vector3 &position );

	// If possible, return an AABB in local coordinates
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );

	// Get the local to world transform
	void GetLocalToWorldTransform( Matrix &transform ) const;

private:
	PEveConnectorVector m_connectors;
	EveCurveLineSetPtr m_lineSet;
};

TYPEDEF_BLUECLASS( EveLineContainer );

#endif