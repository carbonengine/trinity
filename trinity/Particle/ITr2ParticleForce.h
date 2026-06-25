// Copyright © 2023 CCP ehf.

#pragma once
#ifndef ITr2ParticleForce_H
#define ITr2ParticleForce_H

struct ITr2DebugRenderer2;

BLUE_INTERFACE( ITr2ParticleForce ) :
	public IRoot
{
	virtual void Update( float dt ) = 0;
	virtual XMVECTOR FASTCALL GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass ) = 0;
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& worldTransform, const CcpMath::AxisAlignedBox& aabb ) const
	{
	}
};

BLUE_DECLARE_IVECTOR( ITr2ParticleForce );

#endif // ITr2ParticleForce_H