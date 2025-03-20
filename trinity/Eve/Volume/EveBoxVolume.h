////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#pragma once

#include "StdAfx.h"
#include "IEveVolume.h"
#include "Tr2DebugRenderer.h"
BLUE_DECLARE_INTERFACE( IWorldPosition );


BLUE_CLASS( EveBoxVolume ) :
	public IEveVolume,
	public IInitialize,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveBoxVolume( IRoot* lockobj = NULL );
	~EveBoxVolume();

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveVolume
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& parentTransform, const Color& baseColor ) override;
	float GetIntensity( Vector3 position ) override;
	const CcpMath::Sphere GetBoundingSphere() const override;
	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

private:

	void Setup();
	BlueSharedString m_name;

	Vector3 m_position;
	Vector3 m_scaling;
	Vector3 m_innerScaling;
	Quaternion m_rotation;
	bool m_debugShowIntersection;

	Matrix m_boxTransform;
	Matrix m_innerBoxTransform;
	Matrix m_inverseBoxTransform;
	Matrix m_inverseInnerBoxTransform;

	Vector3 m_innerIntersection;
	Vector3 m_outerIntersection;

	CcpMath::Sphere m_boundingSphere;

	static const Vector3 MAX_AABB;
	static const Vector3 MIN_AABB;
};

TYPEDEF_BLUECLASS( EveBoxVolume );
