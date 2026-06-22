// Copyright © 2015 CCP ehf.

#pragma once
#ifndef Tr2ScalarFader_H
#define Tr2ScalarFader_H

// forwards
class EveUpdateContext;

BLUE_CLASS( Tr2ScalarFader ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2ScalarFader( IRoot* lockobj = NULL );
	~Tr2ScalarFader();

	// update
	void Update( const EveUpdateContext& updateContext );

	// start/stop fade
	void StartFade( bool isFadeIn, float fadeLength );

	// access
	bool IsZero() const;
	float GetFaderValue() const;
	bool IsKickInZero() const;
	float GetKickInValue() const;

private:
	// data
	float m_value;
	float m_fading;
	float m_fadeTime;
	float m_kickInLength;
};

TYPEDEF_BLUECLASS( Tr2ScalarFader );

#endif