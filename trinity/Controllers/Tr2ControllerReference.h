// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2Controller.h"

BLUE_CLASS( Tr2ControllerReference ) :
	public ITr2Controller,
	public INotify,
	public IInitialize
{
public:
	Tr2ControllerReference( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual bool Initialize();

	virtual bool OnModified( Be::Var * value );

	virtual void Link( IRoot & owner );
	virtual void Unlink( UnlinkReason reason = UnlinkReason::UNLINKING );
	bool IsLinked() const override;
	virtual void Start();
	virtual void Stop();
	virtual void Update( float normalizedUpdateFrequency );
	virtual void SetVariable( const char* name, float value );
	void HandleEvent( const char* eventName ) override;

	IRoot* GetOwner() const;

private:
	ITr2ControllerPtr m_controller;
	std::string m_path;
	IRoot* m_owner;
};

TYPEDEF_BLUECLASS( Tr2ControllerReference );