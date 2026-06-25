// Copyright © 2018 CCP ehf.

#pragma once

BLUE_DECLARE_INTERFACE( ITr2ActionController );


BLUE_INTERFACE( ITr2ControllerAction ) :
	public IRoot
{
	virtual void Link( ITr2ActionController & controller ){};
	virtual void Unlink() {};
	virtual void Start( ITr2ActionController & controller )
	{
	}
	virtual void Stop( ITr2ActionController & controller )
	{
	}
	virtual void RebaseSimTime( Be::Time diff ) {};
	virtual bool CanTransition() const
	{
		return true;
	}
};

BLUE_DECLARE_IVECTOR( ITr2ControllerAction );
