////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once

BLUE_DECLARE( Tr2StateMachine );
BLUE_DECLARE_INTERFACE( ITr2ControllerAction );
BLUE_DECLARE_IVECTOR( ITr2ControllerAction );
BLUE_DECLARE( Tr2StateMachineTransition );
BLUE_DECLARE_VECTOR( Tr2StateMachineTransition );
BLUE_DECLARE_INTERFACE( ITr2StateMachineStateFinalizer );


BLUE_CLASS( Tr2StateMachineState ) :
	public IListNotify,
	public INotify
{
public:
	Tr2StateMachineState( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual bool OnModified( Be::Var* value );
	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list );

	void Link( const Tr2StateMachine& stateMachine );
	void Unlink();

	void Start();
	void Stop();
	Tr2StateMachineState* Update( uint64_t variableDirtyMask );

	void RebaseSimTime( Be::Time diff );

	const std::string& GetName() const;
	const Tr2StateMachine* GetStateMachine() const;
	void UpdateVariableMask() const;
private:
	Tr2StateMachineState* GetNextState() const;
	IRoot* GetStateMachinePtr() const;

	std::string m_name;
	PITr2ControllerActionVector m_actions;
	PTr2StateMachineTransitionVector m_transitions;
	ITr2StateMachineStateFinalizerPtr m_finalizer;

	const Tr2StateMachine* m_stateMachine;
	mutable uint64_t m_transitionVariableMask;
	bool m_isActive;
	bool m_isFinalizing;
	bool m_hasBeenVetoed;
};

TYPEDEF_BLUECLASS( Tr2StateMachineState );
BLUE_DECLARE_VECTOR( Tr2StateMachineState );
