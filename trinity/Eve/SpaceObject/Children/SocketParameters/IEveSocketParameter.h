// Copyright © 2019 CCP ehf.

#pragma once
#ifndef IEveSocketParameter_H
#define IEveSocketParameter_H

BLUE_DECLARE( Tr2ExternalParameter );

BLUE_INTERFACE( IEveSocketParameter ) :
	public IInitialize
{
	virtual const char* GetName() const
	{
		return "";
	};
	virtual void SetName( const char* name ) {};

	virtual bool Initialize()
	{
		return true;
	};

	virtual void ClearBindings() {};
	virtual bool BindToExternalParameter( Tr2ExternalParameter & externalParameter )
	{
		return true;
	};
	virtual void Reset() {};
	virtual void SetValueToDefault() {};
	virtual bool Used() const
	{
		return true;
	}
	virtual void Propagate() {};
};

BLUE_DECLARE_IVECTOR( IEveSocketParameter );

#endif