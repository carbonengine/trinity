// Copyright © 2018 CCP ehf.

#pragma once

#include "ccpparser.h"

BLUE_DECLARE( Tr2ExpressionTermInfo );
BLUE_DECLARE_INTERFACE( ITr2Updateable );

enum class UnlinkReason
{
	UNLINKING,
	DELETING
};

BLUE_INTERFACE( ITr2Controller ) :
	public IRoot
{
	// Called when a controller is attached to the owning object. The owner makes sure that Link is
	// called before any other method on the controller.
	virtual void Link( IRoot & owner )
	{
	}
	// Called when a controller is detached from the owning object. The controller should clean up
	// any references to the owner here.
	virtual void Unlink( UnlinkReason reason = UnlinkReason::UNLINKING )
	{
	}
	// Returns if the controller already linked to its owner
	virtual bool IsLinked() const = 0;

	// Called when the controller needs to start controlling the owner.
	virtual void Start()
	{
	}
	// Called when the controller needs to stop controlling the owner.
	virtual void Stop()
	{
	}
	// Called every frame between Start and Stop calls.
	virtual void Update( float normalizedUpdateFrequency )
	{
	}

	// Sets controller variable to a new value.
	virtual void SetVariable( const char* name, float value )
	{
	}

	// Handle an instanteous event
	virtual void HandleEvent( const char* eventName )
	{
	}
};

// A controller that supports controller actions
BLUE_INTERFACE( ITr2ActionController ) :
	public ITr2Controller
{
	// Returns the owner of this controller
	virtual IRoot* GetOwner() const = 0;

	// Notifies the controller of a named event
	virtual void Callback( BlueSharedString callbackName ) = 0;

	// Registers an updateable object to be updated when the controller updates.
	virtual void RegisterUpdateable( ITr2Updateable & updateable ) = 0;
	// Unregisters an updateable object.
	virtual void UnRegisterUpdateable( ITr2Updateable & updateable ) = 0;

	// Returns named root objects for dynamic binding paths
	virtual const std::vector<std::pair<std::string, IRoot*>>& GetBindingPathRoots() const = 0;
	virtual std::optional<float> GetFloatVariableByName( const char* name ) const = 0;

	// Populates experession term info with additional functions and variables available to expressions
	virtual void GetExpressionTermInfo( std::vector<Tr2ExpressionTermInfoPtr> & out ) const = 0;


	virtual CcpParser::VariableView GetVariableView() const = 0;
	virtual void* GetVariableBuffer() const = 0;
	virtual void EnsureTempArenaSize( size_t size ) const = 0;
	virtual void* GetTempArena() const = 0;
};
