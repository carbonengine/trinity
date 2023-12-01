////////////////////////////////////////////////////////////
//
//    Created:   November 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef Tr2ScalarFader_H
#define Tr2ScalarFader_H

// forwards
BLUE_DECLARE( EveUpdateContext );

BLUE_CLASS( Tr2ScalarFader ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2ScalarFader( IRoot* lockobj = NULL );
	~Tr2ScalarFader();

	// update
	void Update( EveUpdateContext& updateContext );

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