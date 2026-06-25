// Copyright © 2020 CCP ehf.

#pragma once

#include "StdAfx.h"
#include "IEveVolume.h"
#include "Tr2DebugRenderer.h"

BLUE_CLASS( EveEllipsoidVolume ) :
	public IEveVolume,
	public INotify,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveEllipsoidVolume( IRoot* lockobj = NULL );
	~EveEllipsoidVolume();

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

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

private:
	void Setup();

	BlueSharedString m_name;

	Vector3 m_position;
	Vector3 m_shape;
	Vector3 m_innerShape;
	Quaternion m_rotation;

	Vector3 m_innerIntersection;
	Vector3 m_outerIntersection;
	bool m_debugShowIntersection;

	Matrix m_rotationMatrix;
	Matrix m_inverseRotationMatrix;

	CcpMath::Sphere m_boundingSphere;

	std::map<int, std::function<void()>> m_onChangeCallbacks;
	uint32_t m_nextCallbackID;
};

TYPEDEF_BLUECLASS( EveEllipsoidVolume );