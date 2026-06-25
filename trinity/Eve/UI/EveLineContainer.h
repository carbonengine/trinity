// Copyright © 2015 CCP ehf.

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

	void Update( const EveUpdateContext& context );

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	void UpdateSyncronous( const EveUpdateContext& updateContext )
	{
		Update( updateContext );
	}
	void UpdateAsyncronous( const EveUpdateContext& updateContext )
	{
	}
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform );
	void GetRenderables( std::vector<ITr2Renderable*> & renderables, Tr2ImpostorManager * impostors );
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateModelCenterWorldPosition( Vector3 & position, Be::Time t );
	void GetModelCenterWorldPosition( Vector3 & position ) const;
	bool GetLocalBoundingBox( Vector3 & min, Vector3 & max );
	void GetLocalToWorldTransform( Matrix & transform ) const;

private:
	BlueSharedString m_name;
	PEveConnectorVector m_connectors;
	EveCurveLineSetPtr m_lineSet;

	bool m_display;
};

TYPEDEF_BLUECLASS( EveLineContainer );

#endif