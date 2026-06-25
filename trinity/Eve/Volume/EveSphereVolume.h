// Copyright © 2020 CCP ehf.

#pragma once

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
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& parentTransform, const Color& baseColor ) override;
	float GetIntensity( Vector3 position ) override;
	uint32_t RegisterForChanges( const std::function<void()>& callBack ) override;
	void UnregisterForChanges( uint32_t callbackID ) override;
	void GeneratePointsInVolume( std::vector<Vector3> & points, size_t howManyToAdd, bool excludeInnerVolume, float fallOffFactor ) override;
	const CcpMath::Sphere GetBoundingSphere() const override;

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * val );

private:
	BlueSharedString m_name;

	std::map<uint32_t, std::function<void()>> m_onChangeCallbacks;
	uint32_t m_nextCallbackID;

	CcpMath::Sphere m_outerSphere;
	CcpMath::Sphere m_innerSphere;
};

TYPEDEF_BLUECLASS( EveSphereVolume );
