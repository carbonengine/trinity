// Copyright © 2018 CCP ehf.

#pragma once

#include "Tr2ControllerExpression.h"


BLUE_DECLARE( Tr2StateMachineState );
BLUE_DECLARE( Tr2ExpressionTermInfo );


BLUE_CLASS( Tr2StateMachineTransition ) :
	public INotify
{
public:
	Tr2StateMachineTransition( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual bool OnModified( Be::Var * value );

	void Link( const Tr2StateMachineState& state );
	void Unlink();

	bool CanActivate( uint64_t variableDirtyMask ) const;
	Tr2StateMachineState* GetDestination() const;
	uint64_t GetVariableMask() const;

	IRoot* GetSource() const;

	bool IsConditionValid() const;

	std::vector<Tr2ExpressionTermInfoPtr> GetExpressionTermInfo() const;
	BlueStdResult EvaluateExpression( const char* expression, float& value ) const;

private:
	void UpdateDestination();
	bool IsExpressionValid( const char* attributeName ) const;

	std::string m_destinationName;
	std::string m_condition;

	BlueWeakRef<Tr2StateMachineState> m_destination;
	Tr2ControllerExpression m_evaluator;
	const Tr2StateMachineState* m_source;
};

TYPEDEF_BLUECLASS( Tr2StateMachineTransition );
BLUE_DECLARE_VECTOR( Tr2StateMachineTransition );
