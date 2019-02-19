#pragma once
#ifndef EveBoxVolume_H
#define EveBoxVolume_H

#include "StdAfx.h"
#include "IEveVolume.h"
#include "Tr2DebugRenderer.h"

BLUE_CLASS( EveBoxVolume ) :
	public IEveVolume,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveBoxVolume( IRoot* lockobj = NULL );
	~EveBoxVolume();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveVolume
	void RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& parentTransform ) override;
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
	Vector3 m_scaling;
	Vector3 m_innerScaling;
	Quaternion m_rotation;

	Matrix m_boxTransform;
	Matrix m_centerTransform;
	Matrix m_inverseBoxTransform;
	Matrix m_inverseCenterTransform;

	std::function<void()> m_notifyParentFunc;
	bool m_notifyParent;
};

TYPEDEF_BLUECLASS( EveBoxVolume );

#endif