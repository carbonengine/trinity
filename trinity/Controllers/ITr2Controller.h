////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once


BLUE_INTERFACE( ITr2Controller ) : public IRoot
{
	// Called when a controller is attached to the owning object. The owner makes sure that Link is
	// called before any other method on the controller.
	virtual void Link( IRoot& owner ) {}
	// Called when a controller is detached from the owning object. The controller should clean up
	// any references to the owner here.
	virtual void Unlink() {}
	// Returns if the controller already linked to its owner
	virtual bool IsLinked() const = 0;

	// Called when the controller needs to start controlling the owner.
	virtual void Start() {}
	// Called when the controller needs to stop controlling the owner.
	virtual void Stop() {}
	// Called every frame between Start and Stop calls.
	virtual void Update() {}

	// Sets controller variable to a new value.
	virtual void SetVariable( const char* name, float value ) {}

	// Handle an instanteous event
	virtual void HandleEvent( const char* eventName ) {}
};
