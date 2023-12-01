#pragma once

BLUE_INTERFACE( IEveFxAttribute ) :
	public IRoot
{
	virtual void UpdateAsyncronous( EveUpdateContext & updateContext, const EveChildUpdateParams& params ) = 0;
};

BLUE_DECLARE_IVECTOR( IEveFxAttribute );