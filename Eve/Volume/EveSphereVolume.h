#pragma once
#ifndef EveSphereVolume_H
#define EveSphereVolume_H

#include "StdAfx.h"
#include "IEveVolume.h"
#include "Tr2DebugRenderer.h"

BLUE_CLASS( EveSphereVolume ) :
	public IEveVolume,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveSphereVolume( IRoot* lockobj = NULL );
	~EveSphereVolume();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveVolume
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform ) override;
	float GetIntensity( Vector3 cameraPosition ) override;
	Vector4 GetBoundingSphere() const override;
	void RegisterForChanges( std::function<void()> NotifyParent ) override;

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

private:
	BlueSharedString m_name;

	Vector3 m_position;
	Vector3 m_centerOffset;
	float m_radius;
	float m_innerRadius;

	std::function<void()> m_notifyParentFunc;
	bool m_notifyParent;

};

TYPEDEF_BLUECLASS( EveSphereVolume );

#endif