////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once

BLUE_DECLARE( Tr2Controller );
BLUE_DECLARE( Tr2StateMachineState );
BLUE_DECLARE_VECTOR( Tr2StateMachineState );



BLUE_CLASS( Tr2StateMachine ) :
	public IListNotify,
	public ISimTimeRebaseNotify,
	public INotify
{
public:
	Tr2StateMachine( IRoot* lockobj = nullptr );
	~Tr2StateMachine();

	EXPOSE_TO_BLUE();

	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list );

	virtual bool OnModified( Be::Var* value );

	virtual void OnSimClockRebase( Be::Time oldTime, Be::Time newTime );

	void Link( Tr2Controller& controller );
	void Unlink();

	void Start();
	void Stop();
	void Update( uint64_t variableDirtyMask );

	Tr2Controller* GetController() const;
	Tr2StateMachineState* GetStateByName( const char* name ) const;

	float GetMachineRunTime() const;
	float GetStateRunTime() const;
private:
	void FollowTransitions( uint64_t variableDirtyMask );
	std::string m_name;
	PTr2StateMachineStateVector m_states;
	Tr2StateMachineStatePtr m_startState;

	Tr2StateMachineStatePtr m_currentState;
	Tr2Controller* m_controller;

	Be::Time m_startTime;
	Be::Time m_stateStartTime;
};

TYPEDEF_BLUECLASS( Tr2StateMachine );
BLUE_DECLARE_VECTOR( Tr2StateMachine );
