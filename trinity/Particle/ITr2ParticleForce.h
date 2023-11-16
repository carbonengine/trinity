#pragma once
#ifndef ITr2ParticleForce_H
#define ITr2ParticleForce_H

BLUE_INTERFACE( ITr2ParticleForce ) : public IRoot
{
	virtual void Update( float dt ) = 0;
	virtual XMVECTOR FASTCALL GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass ) = 0;
};

BLUE_DECLARE_IVECTOR( ITr2ParticleForce );

#endif // ITr2ParticleForce_H