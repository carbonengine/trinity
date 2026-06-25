// Copyright © 2013 CCP ehf.

#pragma once
#ifndef Tr2PresentParameters_h
#define Tr2PresentParameters_h

BLUE_CLASS( Tr2PresentParameters ) :
	public IRoot,
	public Tr2PresentParametersAL
{
public:
	EXPOSE_TO_BLUE();

	Tr2PresentParameters( IRoot* lockobj = nullptr );
};

TYPEDEF_BLUECLASS( Tr2PresentParameters );

#endif // Tr2PresentParameters_h