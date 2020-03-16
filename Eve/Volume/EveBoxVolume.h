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
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform ) override;
	float GetIntensity( Vector3 position ) override;
	Vector4 GetBoundingSphere() const override;
	void RegisterForChanges( std::function<void()> NotifyParent ) override;

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

private:

	void Setup();
	BlueSharedString m_name;

	Vector3 m_position;
	Vector3 m_centerOffset;
	Vector3 m_scaling;
	Vector3 m_innerScaling;
	Quaternion m_rotation;
	bool m_debugShowIntersection;

	Matrix m_inverseRotation;
	Matrix m_rotationMatrix;
	Matrix m_boxTransform;
	Matrix m_innerBoxTransform;

	Vector3 m_innerIntersection;
	Vector3 m_outerIntersection;

	std::function<void()> m_notifyParentFunc;
	bool m_notifyParent;
};

TYPEDEF_BLUECLASS( EveBoxVolume );
