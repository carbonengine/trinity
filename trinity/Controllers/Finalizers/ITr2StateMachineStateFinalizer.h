////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once


BLUE_DECLARE( Tr2Controller );


BLUE_INTERFACE( ITr2StateMachineStateFinalizer ) : public IRoot
{
	virtual void Link( Tr2Controller& controller ) {};
	virtual void Unlink() {};
	virtual bool CanTransition( Tr2Controller& controller ) const = 0;
};
